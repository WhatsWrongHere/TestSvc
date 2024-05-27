#include <iostream>
#include <memory>                                           //smart and unique ptr
#include <vector>
#include <string>
#include <utility>                                          //std::pair
#include <regex>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <thread>
#include <sstream>
#include <unordered_map>


#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"		        //logging staff
#include "CLI/CLI11.hpp"                                    //CLI stuff
#define CURL_STATICLIB
#include "curl/curl.h"                                      //URL stuff
#include "RapidXML/rapidxml.hpp"
#include "RapidXML/rapidxml_print.hpp"                      //XML stuff
#include "nlohmann/json.hpp"


#include <windows.h>
#include <tchar.h>                                          //TCHAR stuff
#include <fileapi.h>
#include <shlobj.h>
#include <shlwapi.h>			                            //filesystem stuff
#include "strsafe.h"			                            //StringCbPrintf

#pragma comment(lib, "Shlwapi.lib")



constexpr int gMaxLoggerFileSize{ 5* 1024 * 1024 };				//5 MB
constexpr int gFetchRateByDefault{ 1440 };                      // 1440 times per day (every minute)
const std::vector<std::string> gCurrenciesByDefault{ "EUR, USD" };
const std::vector<std::string> gRequestParamsByDefualt{ "exchangedate", "cc", "rate" };
const std::string gExtensionByDefault{ "json" };
using Limits = std::pair<int, int>;
Limits gFetchRateBounds{ 1,2480 };                              // minimunum: one time per day, max: two times per minute


const std::vector<std::string> gAllowedCurrs{ {"AUD", "CAD", "CNY", "HRK", "CZK", "DKK", "HKD", "HUF", "INR",
                                                "IDR", "ILS", "JPY", "KZT", "KRW", "MXN", "MDL", "NZD", "NOK",
                                                "SAR", "SGD", "ZAR", "SEK", "CHF", "EGP", "GBP", "USD", "BYN", "RON",
                                                "TRY", "XDR", "BGN", "EUR", "PLN", "DZD", "BDT", "AMD", "IRR", "IQD",
                                                "KGS", "LBP", "LYD", "MYR", "MAD", "PKR", "VND", "THB", "AED", "TND",
                                                "UZS", "TMT", "RSD", "AZN", "TJS", "GEL", "BRL", "XAU", "XAG", "XPT",
                                                "XPD"} };
const std::vector<std::string> gAllowedRequestParams{ "exchangedate", "r030", "cc","txt", "enname","rate", "units", "rate_per_unit", "group","calcdate" };
const std::vector<std::string> gAllowedExtensions{ "xml","json", "csv" };
const std::string gNBUInterfrace{ "https://bank.gov.ua/NBU_Exchange/exchange_site?" };
const std::vector<std::string> gAllowedSaveModes{ "different_files", "single_file" };


#define SVCNAME TEXT("TestSvc")
SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle{ nullptr };
HANDLE                  ghSvcStopEvent{ nullptr };

// for WorkThread control and notifications
std::atomic<bool> gStopFlag{ false };
std::condition_variable gCV{  };
std::mutex gCV_m;





using Logger = std::shared_ptr<spdlog::logger>;
class ProgramConfiguration;
enum class ConsoleState;    
enum class ControlState;
enum class SaveMode;



Logger LoggerInit(const std::string& path, const std::string& fileName, const std::string& where, const std::size_t fileSize);                          
void LoggerEnd(Logger& logger);
void LogERRORToEventViewer(const std::wstring& message);                                                                                                
bool IsSvcInstalled(const Logger& logger);
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv);
template <typename T>
bool IsSubset(std::vector<T> subSet, std::vector<T> set);
bool IsFileExists(const std::string& filePath);
bool IsDirectoryExists(const std::string& directoryPath);
bool IsServiceInstalled(const Logger& logger);
// replaces "/" with "//" or do nothing, if they already there
std::string ReparePath(const std::string inputPath);
// seeks for Service's folder at the Program files, and if there isn't, creates, returns "" if can't
std::wstring GetServiceFolder(const Logger& logger);
// seeks for Service's folder at the Program files, and if there isn't, creates but if error, returns "" if can't. uses the LogERRORToEvenViewer to log error
std::wstring GetServiceFolder();
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl);
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
// main loop to control the workThreadExecution
void SvcMainThread(const Logger& logger);
// loop to perform get and saving work
void WorkThread();
// writes data to bytes from URL site
bool GetBinaryDataFrom(const std::string& site, std::stringstream& bytes, const Logger& logger);
// to write callback wrom CURL
std::size_t writeCallback(char* ptr, std::size_t size, std::size_t nmemb, std::stringstream& stream);
// to save data from NBU API stored in bytes vector to files into m_saveDirectory from ProgramConfiguration  
void SaveTable(const std::vector<std::string>& bytesVec, const Logger& logger);
// to convert binary data representing wide string to wide string
std::wstring BytesToWide(const std::string& bytes);
// to refuce XML file, deleting unintended items from XML (cc, txt, etc if they aren't intended (which wasn't specified))
bool ReduceXML(rapidxml::xml_document<>& doc, const std::vector<std::string>& paramsToSave, const Logger& logger);
// write intended XML file to stream
bool XMLToStream(std::vector<std::string>& utf8Strings, std::stringstream& sStreamToWrite, const Logger& logger);
// write intended JSON to stream
bool JSONToStream(const std::vector<std::string>& utf8Strings, std::stringstream& sStreamToWrite, const Logger& logger);
// parse JSONs to Save intended (specified items to CSV files)
bool JSONToCSVStream(const std::vector<std::string>& utf8Strings, std::stringstream& sStreamToWrite, const Logger& logger, bool doWriteSeparator);
// save data from dataToSave fo fileName
bool SaveToFile(const std::string& fileName, const std::wstring& dataToSave, const Logger& logger);
// installs service and starts if specified
bool SvcInstall(const Logger& logger);
std::string GetFileExtension(const std::string& fileName)
{
    size_t dotPosition{ fileName.find_last_of(".") };
    return fileName.substr(dotPosition + 1);
}
// signals SCM to start the service
bool StartSvc(const Logger& logger);
// stops the service, then uninstalls and deletes the svc folder (at least if --save_the_svc_dir wasn't specified)
bool SvcUninstall(const Logger& logger);
// signals SCM to stop the service
bool StopSvc(const Logger& logger);
// handles controll requests from console
bool SvcControl(const Logger& logger);
// throws exceptions, if can't do something and returns status otherwise
SERVICE_STATUS_PROCESS GetServiceStatus();
// check, whether service running or not, if somehing when wrong when check, returns false, and writes to log, what was wrong
bool IsRunning(const Logger& logger);
// check, whether service running or not, if somehing when wrong when check, returns false, and writes to log, what was wrong
bool IsStopped(const Logger& logger);
bool DeleteDirectory(const std::wstring& directoryPath, const Logger& logger);
// different files mode
void SaveTableDifferentMode(const std::vector<std::string>& bytesVec, const Logger& logger);
// single file mode
void SaveTableSingleMode(const std::vector<std::string>& bytesVec, const Logger& logger);
// returns empty string if can't
std::string StringFromFile(const std::string& fileName, const Logger& logger);

// to store which state will intended when console start with arguments
enum class ConsoleState
{
    Install,
    Uninstall,
    Control,
    HelpRequested,
    Unexpected,
    Default,
};
// to store, whether we need to start, stop or do nothing with service execution when control control run
enum class ControlState
{
    Start,
    Stop,
    Default,
};
// to store, wheter we need to save tables on each request to NBU in different file, or write to already existing
enum class SaveMode
{
    SingleFile,
    DifferentFiles,
    Default
};
class ProgramConfiguration
{
public:
    static ProgramConfiguration& GetInstance()
    {
        static ProgramConfiguration instance{};
        return instance;
    }
    int GetFetchRate() { return m_fetchRate; }
    const std::vector<std::string>& GetCurrs() { return m_currencies; }
    const std::vector<std::string>& GetReqParams() { return m_requestParameters; }
    const std::string& GetExtension() { return m_extension; }
    const std::string& GetConfigFilePath() { return m_configFilePath; }
    const std::string& GetSavePath() { return m_savePath; }
    bool DoSaveSvcFolderQ() { return m_SaveFolderAfterUninstall; }
    ConsoleState GetConsoleState() { return m_consoleState; }
    ControlState GetControlState() { return m_controlState; }
    const std::unordered_map<std::string, bool>& GetSettedOptions() { return m_settedOptionsMap; }
    SaveMode GetSaveMode() { return m_saveMode; }
    const std::string& GetSaveFile() { return m_saveFile; }
    const std::string SaveModeToText()
    {
        if (m_saveMode == SaveMode::DifferentFiles)
            return "different_files";
        else if (m_saveMode == SaveMode::SingleFile)
            return "single_file";
        else
            return "default";
    }
    SaveMode TextToSaveMode(const std::string& textToSaveMode)
    {
        if (textToSaveMode == "different_files")
            return SaveMode::DifferentFiles;
        else if (textToSaveMode == "single_file")
            return SaveMode::SingleFile;
        else
            return SaveMode::Default;
    }

    // sets fetchRate, but if it isn't in between gLimits, will set to default
    void SetFetchRate(int fetchRateToSet) 
    {   
        if (fetchRateToSet < gFetchRateBounds.first || fetchRateToSet > gFetchRateBounds.second)
            m_fetchRate = gFetchRateByDefault;
        else
            m_fetchRate = fetchRateToSet; 
    }
    // sets currencies, but if it isn't in gAllowedCurrencies, will set to default
    void SetCurrs(const std::vector<std::string>& currsToSet)
    { 
        if (IsSubset(currsToSet, gAllowedCurrs))
            m_currencies = currsToSet;
        else
            m_currencies = gCurrenciesByDefault;
    }
    // sets request params, but if it isn't in gAllowedRequestParams, will set to default
    void SetReqParams(const std::vector<std::string>& paramsToSet)
    {
        if (IsSubset(paramsToSet, gAllowedRequestParams))
            m_requestParameters = paramsToSet;
        else
            m_currencies = gRequestParamsByDefualt;
    }
    // sets Extesion, but if it isn't in gAllowedExtensions, will set to default
    void SetExtension(const std::string& extensionToSet)
    {
        if (IsSubset({ extensionToSet }, gAllowedExtensions))
            m_extension = extensionToSet;
        else
            m_extension = gExtensionByDefault;
    }
    // sets config file path, but if it isn't existing or not ".ini", will set to "default"
    void SetConfigFilePath(const std::string& confPathToSet)
    {
        if (!IsFileExists(confPathToSet))
        {
            m_configFilePath = "default";
            return;
        }  
        size_t dotPosition{ confPathToSet.find_last_of(".") };
        if(GetFileExtension(confPathToSet) != "ini")
        {
            m_configFilePath = "default";
            return;
        }
        m_configFilePath = m_configFilePath;
    }
    // sets save path, but if it isn't existing, will set to "default"
    void SetSavePath(const std::string& savePathToSet)
    {
        if (!IsDirectoryExists(savePathToSet))
        {
            m_savePath = "default";
            return;
        }
        else
            m_savePath = savePathToSet;
    }

    void SetSaveMode(SaveMode saveMode) { m_saveMode = saveMode; }
    // sets save file if it existing and file extension matches m_extension
    void SetSaveFile(const std::string& fileName )
    {
        if (!IsFileExists(fileName))
        {
            m_saveFile = "none";
            return;
        }
        if (GetFileExtension(fileName) != m_extension)
        {
            m_saveFile = "none";
            return;
        }
        else
            m_saveFile = fileName;
    }

    // set all members (configuration from console)
    void SetFromConsole(int argc, char** argv, const Logger& logger)
    {
        CLI::App app{ CLI::narrow(SVCNAME) };
        argv = app.ensure_utf8(argv);
        logger->trace("SetFromConsole entry with: ");

        for (int iii{ 0 }; iii < argc; ++iii)
            logger->info(argv[iii]);
        bool install{ false };
        bool uninstall{ false };
        bool control{ false };
        bool start{ false };
        bool stop{ false };
        bool singleFileMode{ false };
        bool differentFilesMode{ false };
        bool noneFile{ false };

        int numberOfTimes{ 1 };

        auto installFlag{ app.add_flag("-i,--install", install, "To install the service") };
        auto uninstallFlag{ app.add_flag("-u,--uninstall", uninstall, "To uninstall the service") };
        auto controlFlag{ app.add_flag("--control", control, "To control the service") };
        auto startFlag{ app.add_flag("--start", start, "To control the service") };
        auto stopFlag{ app.add_flag("--stop", stop, "To control the service") };
        auto differentFilesFlag{ app.add_flag("--different_files", differentFilesMode, "To set the different files save mode") };
        auto singleFileFlag{ app.add_flag("--single_file", singleFileMode, "To set the single file save mode") };
        auto noneFileFlag{ app.add_flag("--none_file", noneFile, "To set the save file for single file to none") };


        auto fetchRateOpt{ app.add_option("-r, --fetch_rate", m_fetchRate, "To set fetch rate")->required(false)->check(CLI::Range(gFetchRateBounds.first, gFetchRateBounds.second))->expected(numberOfTimes) };
        auto currenciesOpt{ app.add_option("-v, --currency", m_currencies, "To set vals to save")->required(false)->delimiter(',')->check(CLI::IsMember(gAllowedCurrs)) };
        auto extenstionOpt{ app.add_option("-e, --extension", m_extension, "To set extension of file to save")->required(false)->check(CLI::IsMember(gAllowedExtensions))->expected(numberOfTimes) };

        auto reqParamsOpt{ app.add_option("-p, --request_parameters", m_requestParameters, "To set params to save")->required(false)->delimiter(',')->check(CLI::IsMember(gAllowedRequestParams)) };
        auto confFileOpt{ app.add_option("-c, --configuration_file", m_configFilePath, "To set the config path for controll or uninstallation")->required(false)->check(CLI::ExistingFile)->expected(numberOfTimes) };
        auto savePathOption{ app.add_option("-s, --save_path", m_savePath, "To set path where files will be saved in different files mode")->required(false)->check(CLI::ExistingPath)->expected(numberOfTimes) };
        auto removePathOpt{ app.add_flag("--save_svc_dir", m_SaveFolderAfterUninstall, "Remove the svc folder or not for uninstallation")->required(false) };
        auto saveFileOpt{ app.add_option("-f, --file_to_save", m_saveFile,"To specify file to save (append) table")->required(false)->check(CLI::ExistingFile)->expected(numberOfTimes)};
        

        installFlag->excludes(uninstallFlag, controlFlag, removePathOpt, stopFlag);
        uninstallFlag->excludes(installFlag, controlFlag, fetchRateOpt, currenciesOpt, extenstionOpt, reqParamsOpt, confFileOpt, savePathOption, startFlag, stopFlag, differentFilesFlag, singleFileFlag, saveFileOpt, noneFileFlag);
        controlFlag->excludes(installFlag, uninstallFlag, removePathOpt);
        startFlag->excludes(stopFlag);
        differentFilesFlag->excludes(singleFileFlag, saveFileOpt);
        saveFileOpt->excludes(noneFileFlag);

        try
        {
            app.parse(argc, argv);
        }
        catch (const CLI::CallForHelp& h)
        {
            logger->trace("Help was requested");
            app.exit(h);
            m_consoleState = ConsoleState::HelpRequested;
            return;
        }
        catch (const CLI::ParseError& e)
        {
            std::string msg{ "Encountered parse error: " };
            msg += e.what();
            logger->error(msg);
            std::cerr << "Parse error: " << e.what() << "\n";
            m_consoleState = ConsoleState::Unexpected;
            return;
        }

        //we need somehow check whether options was changed from defaults

        if (stop)
            m_controlState = ControlState::Stop;
        if (start)
            m_controlState = ControlState::Start;

        if (differentFilesMode)
            m_saveMode = SaveMode::DifferentFiles;
        if (singleFileMode)
        {
            m_saveMode = SaveMode::SingleFile;
            if (m_saveFile != "none" && GetFileExtension(m_saveFile) != m_extension)
            {
                std::cerr << "unproper file name (extension mismath), set to none, new file will be saved at savePath";
                m_saveFile = "none";
            }

        }


        if (install)
            m_consoleState = ConsoleState::Install;
        else if (uninstall)
            m_consoleState = ConsoleState::Uninstall;
        else if (control)
        {
            // we need to check, which options was setted and save to map
            if (fetchRateOpt->count() == 1)
                m_settedOptionsMap["fetch_rate"] = true;
            if (currenciesOpt->count() >= 1)
                m_settedOptionsMap["currencies"] = true;
            if (reqParamsOpt->count() >= 1)
                m_settedOptionsMap["request_parameters"] = true;
            if (extenstionOpt->count() >= 1)
                m_settedOptionsMap["extension"] = true;
            if (confFileOpt->count() == 1)
                m_settedOptionsMap["configuration_file"] = true;
            if (savePathOption->count() == 1)
                m_settedOptionsMap["save_path"] = true;
            if (differentFilesFlag->count() == 1 || singleFileFlag->count() == 1)
                m_settedOptionsMap["save_mode"] = true;
            if (saveFileOpt->count() == 1)
                m_settedOptionsMap["save_file"] = true;
            if(noneFile)
                m_settedOptionsMap["save_file"] = true;


            m_consoleState = ConsoleState::Control;
        }
            
        else
            m_consoleState = ConsoleState::Unexpected;
        logger->trace("PC::SetFromConsole exiting");
    }
    // saves program configuration (fetchratem currencies, request parameters, extension, config file path ("default", if default) and save files path ("default", if default) 
    void SaveTo(std::ofstream& ofstream)
    {
        ofstream << "fetch_rate = " << m_fetchRate << "\n";
        ofstream << "currencies = ";
        for (int iii{ 0 }; iii < static_cast<int>(m_currencies.size()) - 1; ++iii)
            ofstream << m_currencies[iii] << ", ";
        ofstream << m_currencies.back() << "\n";

        ofstream << "request_parameters = ";
        for (int iii{ 0 }; iii < static_cast<int>(m_requestParameters.size()) - 1; ++iii)
            ofstream << m_requestParameters[iii] << ", ";
        ofstream << m_requestParameters.back() << "\n";

        if (m_configFilePath == "default")
            ofstream << "config_file = default\n";
        else
            ofstream << "configuration_file = " << "\"" << ReparePath(m_configFilePath) << "\"" << "\n";

        if(m_savePath == "default")
            ofstream << "save_path = default\n";
        else
            ofstream << "save_path = " << "\"" << ReparePath(m_savePath) << "\"" << "\n";

        if (m_saveMode == SaveMode::DifferentFiles)
            ofstream << "save_mode = different_files\n";
        else if (m_saveMode == SaveMode::SingleFile)
            ofstream << "save_mode = single_file\n";

        if (m_saveFile == "none")
            ofstream << "save_file = none\n";
        else
            ofstream << "save_file = " << "\"" << ReparePath(m_saveFile) << "\"" << "\n";

        ofstream << "extension = " << m_extension << "\n";
    }
    // seeks for config.ini file at the service's program files dir, if there isn't, it will create one, or returns false when can't;
    bool SaveToDefaultConfigFile(const Logger& logger)
    {
        logger->trace("SaveToDefaultConfigFile entry");
        std::string svcConfigFilePath{ CLI::narrow(GetServiceFolder(logger)) };
        if (svcConfigFilePath == "")
        {
            logger->error("Unable to get the service folder path\n, exiting");
            return false;
        }
        logger->trace("trying to create or open the default config file: ");
        svcConfigFilePath += "\\config.ini";
        logger->info(svcConfigFilePath);
        std::ofstream configFile(svcConfigFilePath);
        if (!configFile.is_open())
        {
            std::cerr << "Error: Unable to create config file\n";
            return false;
        }
        SaveTo(configFile);
        configFile.close();
        logger->trace("Config file written successfully, exiting");
        return true;
    }
    // seeks for file (filePath) if there isn't, it will create one, or returns false when can't;
    bool SaveToConfigFile(const std::string& filePath, const Logger& logger)
    {
        logger->trace("SaveToConfig file entry with filePath:");
        logger->info(filePath);
        logger->trace("trying to create or open the custom config file.");
        std::ofstream configFile(filePath);
        if (!configFile.is_open())
        {
            logger->error("Unable to create the custom config file, exiting");
            return false;
        }
        SaveTo(configFile);
        configFile.close();
        logger->trace("Custom config file written successfully, exiting");
        return true;
    }
    // seeks both default and custom (if existing in current program configuration) configFiles and save the actual configuration there
    bool SaveConfiguration(const Logger& logger)
    {
        logger->trace("Set configuration entry with m_configFilePath: ");
        logger->info(m_configFilePath);
        bool isSuccessfull{ true };
        isSuccessfull = isSuccessfull && SaveToDefaultConfigFile(logger);
        if (m_configFilePath != "default")
            isSuccessfull = isSuccessfull && SaveToConfigFile(m_configFilePath, logger);
        return isSuccessfull;
    }
    /// <summary>
    /// seeks for custom configuration file at the default configuration file (in folder at the ProgramFiles), and if it is existing, it will return it's validity, if there isn't,
    /// it will return validity of default configuration file, if there isn't default configuration file, returns false.
    /// 
    /// File is considered as valid even if it will emty or semi-empty, other options will be setted by default
    /// </summary>
    bool ValidateConfigurationFile(const Logger& logger)
    {
        logger->trace("VaidateConfigurationFile Entry");
        CLI::ConfigINI cfg;
        cfg.arrayDelimiter(',');
        std::vector<CLI::ConfigItem> ConfigItems{};
        std::string serviceFolder{ CLI::narrow(GetServiceFolder(logger)) };
        if (serviceFolder == "")
        {
            logger->error("unable get the service folder path, exiting");
            return false;
        }
        std::string defaultConfigFilePath{ CLI::narrow(GetServiceFolder(logger)) + "\\config.ini" };
        logger->trace("trying to check the defaut configuration file: ");
        logger->info(defaultConfigFilePath);
        try
        {
            ConfigItems = cfg.from_file(defaultConfigFilePath);
        }
        catch (const CLI::ParseError& er)
        {
            //if unable to parse the default config file, we will reset default config file
            std::string msg{ "Configuration parse error encountered: " };
            msg += er.what();
            logger->error(msg);
            return false;
        }
        logger->trace("trying to find the custom config file path");
        std::string configFilePath{"default"};                // no custom config file
        for (const auto& item : ConfigItems)
        {
            if (item.name == "configuration_file")
                configFilePath = item.inputs[0];
        }

        logger->info(configFilePath);
        if (configFilePath != "default")
        {
            if (!IsFileExists(configFilePath))
            {
                logger->error("custom config file isn't exists");
                return false;
            }
            logger->trace("trying to parse the custom config file");
            try
            {
                ConfigItems = cfg.from_file(configFilePath);
            }
            catch (const CLI::ParseError& er)
            {
                //if unable to parse the default config file, we will reset default config file
                std::string msg{ "Configuration parse error encountered: " };
                msg += er.what();
                logger->error(msg);
                return false;
            }
        }
        std::string extToCompare{ m_extension };
        for (const auto& item : ConfigItems)
        {
            if (item.name == "fetch_rate")
                if (std::stoi(item.inputs[0]) <= gFetchRateBounds.first || std::stoi(item.inputs[0]) >= gFetchRateBounds.second)
                {
                    logger->error("unpropper config file, wrong fetchrate: ");
                    logger->info(item.inputs[0]);                                   //we will consider only first parameter
                    return false;
                }

            if (item.name == "currencies")
                if (!IsSubset(item.inputs, gAllowedCurrs))
                {
                    logger->error("unpropper config file, wrong currencies: ");
                    for (const auto& input : item.inputs)
                        logger->info(input);
                    return false;
                }

            if (item.name == "request_parameters")
                if (!IsSubset(item.inputs, gAllowedRequestParams))
                {
                    logger->error("unpropper config file, wrong request parameters: ");
                    for (const auto& input : item.inputs)
                        logger->info(input);
                    return false;
                }
            
            if (item.name == "extension")
                if (!IsSubset({ item.inputs[0] }, gAllowedExtensions))              //we will consider only first parameter
                {
                    logger->error("unpropper config file, wrong extension: ");
                    logger->info(item.inputs[0]);
                    return false;
                }
                else
                    extToCompare = item.inputs[0];

            if (item.name == "save_path")
                if (item.inputs[0] != "default" && !IsDirectoryExists(item.inputs[0]))
                {
                    logger->error("unpropper config file, wrong save path: ");
                    logger->info(item.inputs[0]);
                    return false;
                }
            if(item.name == "save_mode")
                if (!IsSubset({ item.inputs[0] }, gAllowedSaveModes))
                {
                    logger->error("unpropper config file, wrong save mode: ");
                    logger->info(item.inputs[0]);
                    return false;
                }
        }
        for (const auto& item : ConfigItems)                        //we will check for save file in the next cycle for extToCompare to be setted
        {
            if (item.name == "save_file")
                if (item.inputs[0] != "none" && (!IsFileExists(item.inputs[0]) || GetFileExtension(item.inputs[0]) != extToCompare))
                {
                    logger->error("unpropper config file, wrong save file: ");
                    logger->info(item.inputs[0]);
                    logger->info("extension: ");
                    logger->info(extToCompare);
                    logger->info("IsFileExists(item.inputs[0]):");
                    logger->info(IsFileExists(item.inputs[0]));
                    return false;
                }
        }
        logger->trace("ConfigurationFile is propper");
        return true;
    }
    
    /// <summary>
    /// seeks for custom configuration file at the default configuration file (in folder at the ProgramFiles) and if existing, rewrites both of them. 
    /// If there no configuration file, rewrites only default configuration file. It will try to parse custom (if exists) or default (if exists) to catch propper parameters, then files will be will be rerwritten
    /// with captured parameters and current to addition (for thouse ones, which weren't caught at the configuration file).
    /// If there aren't both default and custom configuration files, it will create one (default) with current configuraration from memory.
    /// </summary>
    bool FixConfigurations(const Logger& logger)
    {
        logger->trace("FixConfigurations Entry");
        CLI::ConfigINI cfg;
        cfg.arrayDelimiter(',');
        std::vector<CLI::ConfigItem> ConfigItems{};
        std::string serviceFolder{ CLI::narrow(GetServiceFolder(logger)) };
        if (serviceFolder == "")
        {
            logger->error("unable get the service folder path, exiting");
            return false;
        }
        std::string defaultConfigFilePath{ CLI::narrow(GetServiceFolder(logger)) + "\\config.ini" };
        logger->trace("trying to check the defaut configuration file: ");
        logger->info(defaultConfigFilePath);
        try
        {
            ConfigItems = cfg.from_file(defaultConfigFilePath);
        }
        catch (const CLI::ParseError& er)
        {
            //if unable to parse the default config file, we will reset default config file with current configuration in memory
            std::string msg{ "Configuration parse error encountered: " };
            msg += er.what();
            logger->error(msg);
            logger->trace("So we will rewrite the default one");
            m_configFilePath = "default";
            return SaveToDefaultConfigFile(logger);
        }
        logger->trace("trying to find the custom config file path");
        std::string configFilePath{ "default" };                // no custom config file (if default)
        for (const auto& item : ConfigItems)
        {
            if (item.name == "configuration_file")
                configFilePath = item.inputs[0];
        }
        logger->info(configFilePath);
        if (configFilePath == "default")                        // we can parse default config file
        {
            m_configFilePath = "default";
            SetFromConfigItems(ConfigItems, logger);
            if (!IsDirectoryExists(m_savePath))
                m_savePath = "default";
            logger->trace("FixConfigurations ending");
            return SaveToDefaultConfigFile(logger);
        }

        if (configFilePath != "default")
        {
            if (!IsFileExists(configFilePath))
            {
                logger->trace("custom config file isn't exists, let's check existence of parent path: ");
                m_configFilePath = defaultConfigFilePath;
                configFilePath = defaultConfigFilePath;
            }
            try
            {
                ConfigItems = cfg.from_file(configFilePath);
            }
            catch (const CLI::ParseError& er)
            {
                std::string msg{ "Configuration parse error encountered: " };
                msg += er.what();
                logger->error(msg);
                // if we are here, we have already been able to parse default file, so:
                ConfigItems = cfg.from_file(defaultConfigFilePath);
                logger->trace("We will rewrite both (if both are exists)");
            }
            m_configFilePath = configFilePath;
            SetFromConfigItems(ConfigItems, logger);
            if (!IsDirectoryExists(m_savePath))
                m_savePath = "default";
            logger->trace("FixConfigurations ending");
            return SaveConfiguration(logger);
        }

        logger->trace("Fix Configuration is ending in unexpected way");
        return false;
    }

    /// <summary>
    /// parses the default configuration to find, whether the custom config exists, if so, takes configuration from there, if not, takes configuration from default file. If there semi-full files (not all parameters values specified),
    /// will set from current configuration in memory.
    /// </summary>
    bool SetFromConfiguration(const Logger& logger)
    {
        logger->trace("Set from configuration entry");
        CLI::ConfigINI cfg;
        cfg.arrayDelimiter(',');
        std::vector<CLI::ConfigItem> ConfigItems{};
        std::string defaultConfigFilePath{ CLI::narrow(GetServiceFolder(logger)) + "\\config.ini" };
        logger->trace("trying to check the defaut configuration file: ");
        logger->info(defaultConfigFilePath);
        try
        {
            ConfigItems = cfg.from_file(defaultConfigFilePath);
        }
        catch (const CLI::ParseError& er)
        {
            //if unable to parse the default config file, we will reset default config file
            std::string msg{ "Configuration parse error encountered: " };
            msg += er.what();
            logger->error(msg);
            return false;
        }
        logger->trace("trying to find the custom config file path:");
        std::string configFilePath{ "default" };                // no custom config file
        for (const auto& item : ConfigItems)
        {
            if (item.name == "configuration_file")
                configFilePath = item.inputs[0];
        }
        logger->trace(configFilePath);
        if (configFilePath == "default")
        {
            m_configFilePath = "default";
            SetFromConfigItems(ConfigItems, logger);
            return true;
        }
        try
        {
            ConfigItems = cfg.from_file(configFilePath);
        }
        catch (const CLI::ParseError& er)
        {
            //if unable to parse the default config file, we will reset default config file
            std::string msg{ "Configuration parse error encountered: " };
            msg += er.what();
            logger->error(msg);
            return false;
        }
        m_configFilePath = configFilePath;
        SetFromConfigItems(ConfigItems, logger);
        return true;
    }
    void ResetToDefaults()
    {
        m_fetchRate = gFetchRateByDefault;
        m_currencies = gCurrenciesByDefault;
        m_requestParameters = gRequestParamsByDefualt;
        m_configFilePath = "default";
        m_extension = "xml";
        m_savePath = "default";
        m_consoleState = ConsoleState::Default;
        m_SaveFolderAfterUninstall = false;
        m_saveMode = SaveMode::DifferentFiles;
        m_saveFile = "none";
    }

private:

    int m_fetchRate{ gFetchRateByDefault };
    std::vector<std::string> m_currencies{ gCurrenciesByDefault };
    std::vector<std::string> m_requestParameters{ gRequestParamsByDefualt };
    std::string m_extension{ gExtensionByDefault };
    std::string m_configFilePath{ "default" };                              // config.ini file at service's folder in program files (if default)
    std::string m_savePath{ "default" };                                    // files will be saved at service's folder in program files
    bool m_SaveFolderAfterUninstall{ false };                               // whether save or not the Svc folder path at program files

    ConsoleState m_consoleState{ ConsoleState::Default };
    ControlState m_controlState{ ControlState::Default };

    SaveMode m_saveMode{ SaveMode::DifferentFiles };
    std::string m_saveFile{ "none" };

    std::unordered_map<std::string, bool> m_settedOptionsMap{};
    
    //will take all allowed parameters from config items
    void SetFromConfigItems(const std::vector<CLI::ConfigItem> items, const Logger& logger)
    {
        logger->trace("set from config items entry");
        for (const auto& item : items)
        {
            if (item.name == "fetch_rate")
                if (std::stoi(item.inputs[0]) >= gFetchRateBounds.first && std::stoi(item.inputs[0]) <= gFetchRateBounds.second)
                {
                    logger->info("fetch_rate can be setted from config:");
                    logger->info(item.inputs[0]);                           //we will consider only first parameter
                    m_fetchRate = std::stoi(item.inputs[0]);
                }

            if (item.name == "currencies")
                if (IsSubset(item.inputs, gAllowedCurrs))
                {
                    logger->info("currencies can be setted from config:");
                    for (const auto& input : item.inputs)
                        logger->info(input);
                    m_currencies = item.inputs;
                }

            if (item.name == "request_parameters")
                if (IsSubset(item.inputs, gAllowedRequestParams))
                {
                    logger->info("request_parameters can be setted from config:");
                    for (const auto& input : item.inputs)
                        logger->info(input);
                    m_requestParameters = item.inputs;
                }

            if (item.name == "extension")
                if (IsSubset({ item.inputs[0] }, gAllowedExtensions))              
                {
                    logger->info("extension can be setted from config:");
                    logger->info(item.inputs[0]);
                    m_extension = item.inputs[0];
                }

            if (item.name == "save_path")
                if (IsDirectoryExists(item.inputs[0]))
                {
                    logger->info("save_path can be setted from config:");
                    logger->info(item.inputs[0]);
                    m_savePath = item.inputs[0];
                }
            if (item.name == "config_file")
                if (IsFileExists(item.inputs[0]))
                {
                    logger->info("config_file can be setted from config:");
                    logger->info(item.inputs[0]);
                    m_configFilePath = item.inputs[0];
                }

            if (item.name == "save_mode")
                if (IsSubset({ item.inputs[0] }, gAllowedSaveModes))
                {
                    logger->info("save mode can be setted from config:");
                    logger->info(item.inputs[0]);
                    m_saveMode = TextToSaveMode(item.inputs[0]);
                }
        }
        // will check for "save_file at the separate loop as it depends on m_extension"
        for (const auto& item : items)
        {
            if (item.name == "save_file")
                if (item.inputs[0] == "none" || (IsFileExists(item.inputs[0]) && GetFileExtension(item.inputs[0]) == m_extension))
                {
                    logger->info("save file can be setted from config:");
                    logger->info(item.inputs[0]);
                    m_saveFile = item.inputs[0];
                }
        }  
    }

    ProgramConfiguration() {}
    ~ProgramConfiguration() {}
    ProgramConfiguration(const ProgramConfiguration&) = delete;
    ProgramConfiguration& operator= (const ProgramConfiguration&) = delete;
};




Logger LoggerInit(const std::string& path, const std::string& fileName, const std::string& where, std::size_t fileSize)                                 
{
    Logger logger{ nullptr };
    try
    {
        std::string logFile{ path };
        if(!logFile.empty() && logFile.back() != '\\')
            logFile += "\\";
        logFile += fileName;
        logger = spdlog::rotating_logger_mt(where, logFile, fileSize, 1);          // creating the logger with a single file with fixed max file size
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::wstring message{ L"Failed to initialize logger exceptioin in: " };
        message += CLI::widen(where);
        message += L", what:";
        message += std::wstring{ ex.what(), ex.what() + strlen(ex.what()) };
        LogERRORToEventViewer(message);
        return nullptr;
    }
    return logger;
}
void LoggerEnd(Logger& logger)
{
    logger->flush();
    spdlog::shutdown();
}
void LogERRORToEventViewer(const std::wstring& message)
{
    HANDLE hEventLog = RegisterEventSource(nullptr,				//local PC
                                           SVCNAME);
    if (!hEventLog)
    {
        LPCWSTR strings[1]{ message.c_str() };

        ReportEvent(hEventLog,							        // Event log handle
                    EVENTLOG_ERROR_TYPE,				        // Event type
                    0,									        // Event category
                    0,									        // Event identifier
                    nullptr,							        // No security identifier
                    1,									        // Size of lpszStrings array
                    0,									        // No binary data
                    strings,							        // Array of strings
                    nullptr);							        // No binary data
        DeregisterEventSource(hEventLog);
    }
}
bool IsSvcInstalled(const Logger& logger)
{
    logger->trace("IsServiceInstalled entry");
    SC_HANDLE schSCManager{ OpenSCManager(nullptr,				// local PC
                                          nullptr,				// default database
                                          SC_MANAGER_CONNECT) };
    if (!schSCManager)
    {
        logger->error("Failed to check service installation, failed to open SCM");
        return false;
    }
    SC_HANDLE schService{ OpenService(schSCManager, SVCNAME, SERVICE_QUERY_CONFIG) };
    if (!schService)
    {
        logger->info("There is no such service, failed to OpenService when doing IsServiceInstalled check");
        CloseServiceHandle(schSCManager);
        return false;
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    logger->trace("IsServiceInstalled returning");
    return true;
}
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);
    if (!gSvcStatusHandle)
    {
        LogERRORToEventViewer(L"gSvcStatusHandle failure in SvcMain, returning");
        return;
    }

    std::wstring svcFolderPath{ GetServiceFolder() };
    Logger logger{ LoggerInit(CLI::narrow(svcFolderPath), "ServiceMain.log", "ServicePath", gMaxLoggerFileSize) };
    if (!logger)
    {
        LogERRORToEventViewer(L"Unable to get logger in SvcMain, returning");
        return;
    }
    spdlog::set_level(spdlog::level::trace);
    logger->trace("Service path is started");

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gSvcStatus.dwServiceSpecificExitCode = 0;

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    logger->trace("SvcMain ends here, then starts SvcStart");
    SvcMainThread(logger);
    logger->trace("SvcMain returns");
    LoggerEnd(logger);
}
template <typename T>
bool IsSubset(std::vector<T> subSet, std::vector<T> set)
{
    std::unordered_set<T> uSet(set.begin(), set.end());
    for (const auto& element : subSet)
    {
        if (uSet.find(element) == uSet.end())
            return false;
    }
    return true;
}
bool IsFileExists(const std::string& filePath)
{
    DWORD fileAttributes = GetFileAttributes(CLI::widen(filePath).c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return true;
}
bool IsDirectoryExists(const std::string& directoryPath) 
{
    DWORD fileAttributes{ GetFileAttributes(CLI::widen(directoryPath).c_str()) };
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) 
        return false;

    return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}
bool IsServiceInstalled(const Logger& logger)
{
    logger->trace("IsServiceInstalled entry");
    SC_HANDLE schSCManager{ OpenSCManager(nullptr,				// local PC
                                          nullptr,				// default database
                                          SC_MANAGER_CONNECT) };
    if (!schSCManager)
    {
        logger->error("Failed to check service installation, failed to open SCM");
        return false;
    }
    SC_HANDLE schService{ OpenService(schSCManager, SVCNAME, SERVICE_QUERY_CONFIG) };
    if (!schService)
    {
        logger->info("There is no such service, failed to OpenService when doing IsServiceInstalled check");
        CloseServiceHandle(schSCManager);
        return false;
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    logger->trace("IsServiceInstalled returning");

    return true;
}
std::string ReparePath(const std::string inputPath)
{

    std::regex doubleBackslashPattern(R"(\\\\)");
    if (std::regex_search(inputPath, doubleBackslashPattern))
        return inputPath;

    std::string result;
    result.reserve(inputPath.size() * 2);
    for (char ch : inputPath)
    {
        if (ch == '\\')
            result += "\\\\";
        else
            result += ch;
    }
    return result;
}
std::wstring GetServiceFolder(const Logger& logger)
{
    logger->trace("Entered the GetServiceFolder");
    TCHAR programFilesPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPath(nullptr,
                                  CSIDL_PROGRAM_FILES,
                                  nullptr,						//any user
                                  SHGFP_TYPE_CURRENT,
                                  programFilesPath)))
    {
        std::wstring svcFolderPath{ programFilesPath };
        svcFolderPath += L"\\" SVCNAME;
        logger->info(std::string{ "Target service folder: " } + CLI::narrow(svcFolderPath));
        if (CreateDirectory(svcFolderPath.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS)
        {
            logger->trace("Succeded in folder getting or creation, returning");
            return svcFolderPath;
        }
        else
        {
            logger->error(std::string{ "Failed to create directory. Error code: " } + std::to_string(GetLastError()));
            logger->trace("exiting the GetServiceFolder");
            return L"";
        }
    }
    logger->error(std::string{ "Failed to get Folder path. Error code: " } + std::to_string(GetLastError()));
    logger->trace("exiting the GetServiceFolder");
    return L"";
}
std::wstring GetServiceFolder()			//creates folder when it isn't existing yet
{
    TCHAR programFilesPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr,
                                  CSIDL_PROGRAM_FILES,
                                  nullptr,						//any user
                                  SHGFP_TYPE_CURRENT,
                                  programFilesPath)))
    {
        std::wstring svcFolderPath{ programFilesPath };
        svcFolderPath += L"\\" SVCNAME;
        if (CreateDirectory(svcFolderPath.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS)
            return svcFolderPath;
        else
        {
            LogERRORToEventViewer(std::wstring{ L"Failed to get service directory.Error code: " } + std::to_wstring(GetLastError()));
            return L"";
        }
    }
    LogERRORToEventViewer(std::wstring{ L"Failed to get Folder path. Error code: " } + std::to_wstring(GetLastError())); \
    return L"";
}
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        SetEvent(ghSvcStopEvent);
        ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
        return;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    default:
        break;
    }

}
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint{ 1 };

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}
void SvcMainThread(const Logger& logger)
{
    logger->trace("SvcMainThread entry");
    ghSvcStopEvent = CreateEvent(nullptr,	    // default security attributes
                                 true,			// manual reset event
                                 false,			// not signaled
                                 nullptr);		// no name
    if (!ghSvcStopEvent)
    {
        logger->error("ghSvcStopEvent failure");
        ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    // Report running status when initialization is complete.
    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
    logger->trace("Going to main cycle");
    
    std::thread workThread(WorkThread);
    workThread.detach();
    while (1)
    {
        DWORD waitResult = WaitForSingleObject(ghSvcStopEvent, INFINITE);
        if (waitResult == WAIT_OBJECT_0)
        {
            logger->trace("Stop signal received, signaling main work thread to stop");
            gStopFlag.store(true);
            gCV.notify_all();
            break;
        }
        else
        {
            logger->error("WaitForSingleObject failed with error: ", GetLastError());
            ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
            break;
        }
    }
    if (workThread.joinable())
        workThread.join();

    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
    logger->trace("SvcMainThread ends");
}
void WorkThread()
{
    std::string svcFolderPath{ CLI::narrow(GetServiceFolder()) };
    if (svcFolderPath == "")
    {
        gStopFlag.store(true);
        gCV.notify_all();
        return;
    }
    Logger logger{ LoggerInit(svcFolderPath,"WorkThread.log","WorkThread",gMaxLoggerFileSize) };
    if (!logger)
    {
        gStopFlag.store(true);
        gCV.notify_all();
        return;
    }
    logger->trace("//////////////////////////////////////////////////////////////////////////////");
    logger->trace("start logging");
    ProgramConfiguration& conf{ ProgramConfiguration::GetInstance() };
    bool binaryDataIsSetted{};                                          // to store wheter binary data was setted to the bytesVec;
    std::vector<std::string> bytesVec{};                                // to store binary data from source (NBU API)
    bytesVec.reserve(conf.GetCurrs().size());
    std::stringstream bytes{};                                          // to store write binary data from source (NBU API) and store it in bytesVec then.
    std::string source{};                                               // to create the URL to access the data fron NBU

    while (!gStopFlag.load())
    {
        logger->trace("trying to load the configuratuion");
        if (!conf.ValidateConfigurationFile(logger))
            conf.FixConfigurations(logger);
        if (!conf.SetFromConfiguration(logger))
        {
            logger->error("ecnountered problem when trying to get the configuration from file, we will continue");
            continue;
        }
        if (!conf.SaveConfiguration(logger))                                        // to sync both conf files if they are existring
        {
            logger->error("ecnountered problem when trying save configuration");
            continue;
        }
        logger->trace("configuration loaded successfully");
        logger->trace("Performing...");
        binaryDataIsSetted = true;
        for (const auto& currency : conf.GetCurrs())
        {
            source = gNBUInterfrace;
            source += "&valcode=";
            source += currency;
            source += "&";
            source += (conf.GetExtension() == "xml" || conf.GetExtension() == "json" ? conf.GetExtension() : gExtensionByDefault);
            logger->trace("intended source is:");
            logger->info(source);
            binaryDataIsSetted = binaryDataIsSetted && GetBinaryDataFrom(source, bytes, logger);
            bytesVec.push_back(bytes.str());
        }
        if (binaryDataIsSetted)
            SaveTable(bytesVec, logger);
        else
            logger->info("Unable to get binary data, going to next iteration");
        bytesVec.clear();
        std::unique_lock<std::mutex> lk(gCV_m);
        //60*60*24 - number of seconds in 24 hours
        if (gCV.wait_for(lk, std::chrono::seconds(60*60*24/conf.GetFetchRate()), [] { return gStopFlag.load(); }))
            break;
    }
    logger->trace("WorkThread returns");
    LoggerEnd(logger);
}
bool GetBinaryDataFrom(const std::string& site, std::stringstream& bytes, const Logger& logger)
{
    logger->trace("GetBinaryDataFrom entry");

    bytes.str("");
    CURL* curlHandle{ curl_easy_init() };
    if (!curlHandle)
    {
        logger->error("Unable to init the curlHandler");
        return false;
    }
    CURLcode code{ curl_easy_setopt(curlHandle, CURLOPT_URL, site.c_str()) };
    bool setOptFailure{ false };
    std::string msg{ "Setopt failire: " };
    if (code != CURLE_OK)
    {
        msg += curl_easy_strerror(code);
        setOptFailure = true;
    }
    code = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeCallback);
    if (code != CURLE_OK)
    {
        msg += curl_easy_strerror(code);
        setOptFailure = true;
    }
    code = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &bytes);
    if (code != CURLE_OK)
    {
        msg += curl_easy_strerror(code);
        setOptFailure = true;
    }
    if (setOptFailure)
    {
        logger->error(msg + "going to another iteration");
        curl_easy_cleanup(curlHandle);
        return false;
    }
    CURLcode res{ curl_easy_perform(curlHandle) };
    if (res != CURLE_OK)
    {
        msg = "curl_easy_perform() failed: ";
        msg += curl_easy_strerror(res);
        logger->error(msg);
        curl_easy_cleanup(curlHandle);
        return false;
    }
    logger->trace("GetBinaryDataFrom entry exiting");
    curl_easy_cleanup(curlHandle);
    return true;
}
std::size_t writeCallback(char* ptr, std::size_t size, std::size_t nmemb, std::stringstream& stream)
{
    std::size_t realsize = size * nmemb;
    stream.write(ptr, realsize);
    return size * nmemb;
}
void SaveTable(const std::vector<std::string>& bytesVec, const Logger& logger)
{
    logger->trace(std::string{ "SaveTable entry with Save Mode:" } );
    ProgramConfiguration& conf{ ProgramConfiguration::GetInstance() };
    logger->info(conf.SaveModeToText() + std::string{ " saveFile = " } + conf.GetSaveFile());
    if (conf.GetSaveMode() == SaveMode::DifferentFiles)
        SaveTableDifferentMode(bytesVec, logger);
    else if (conf.GetSaveMode() == SaveMode::SingleFile)
        SaveTableSingleMode(bytesVec, logger);

    logger->trace("SaveTable ended");
}
void SaveTableDifferentMode(const std::vector<std::string>& bytesVec, const Logger& logger)
{
    logger->trace(std::string{ "SaveTableDifferentMode entry" });
    std::time_t now{ std::time(nullptr) };
    std::tm localTimeInfo{};
    char timeBuffer[80];
    std::string fileName;
    ProgramConfiguration& conf{ ProgramConfiguration::GetInstance() };
    localtime_s(&localTimeInfo, &now);
    strftime(timeBuffer, sizeof(timeBuffer), "%d-%m-%Y %H_%M", &localTimeInfo);
    if (conf.GetSavePath() == "default")
        fileName = CLI::narrow(GetServiceFolder(logger));
    else
        fileName = conf.GetSavePath();

    logger->info("target directory:");
    fileName += "\\";
    logger->info(fileName);
    fileName += timeBuffer;
    fileName += "_output.";
    fileName += conf.GetExtension();
    std::vector<std::string> utf8Strings{};
    logger->info(std::string{ "bytesVec.size() = " } + std::to_string(bytesVec.size()));
    if (bytesVec.empty())
        return;
    utf8Strings.reserve(bytesVec.size());
    try
    {
        for (const auto& bytes : bytesVec)
            utf8Strings.push_back(CLI::narrow(BytesToWide(bytes)));
    }
    catch (const std::exception& exc)
    {
        std::string msg{ "error encountered: " };
        msg += exc.what();
        msg += ", exiting";
        logger->error(msg);
        return;
    }
    std::stringstream ss{};
    if (conf.GetExtension() == "xml")
    {
        if (!XMLToStream(utf8Strings, ss, logger))
        {
            logger->error("XMLToStream is failed, SaveTable exiting");
            return;
        }
    }
    else if (conf.GetExtension() == "json")
    {
        if (!JSONToStream(utf8Strings, ss, logger))
        {
            logger->error("JSONToStream is failed, SaveTable exiting");
            return;
        }
    }
    else if (conf.GetExtension() == "csv")
    {
        bool doWriteSeparator{ true };                          //cause it is the first write to the file
        if (!JSONToCSVStream(utf8Strings, ss, logger, doWriteSeparator))
        {
            logger->error("JSONToStream is failed, SaveTable exiting");
            return;
        }
    }
    else
    {
        logger->error("Unexpected file extension encountered, exiting");
        return;
    }
    SaveToFile(fileName, CLI::widen(ss.str()), logger);
    logger->trace("SaveTableDifferentMode ended successfully");
}
void SaveTableSingleMode(const std::vector<std::string>& bytesVec, const Logger& logger)
{
    logger->trace(std::string{ "SaveTableSingleMode entry" });
    ProgramConfiguration& conf{ ProgramConfiguration::GetInstance() };
    std::vector<std::string> utf8Strings{};
    std::stringstream ss{};
    std::string fileName{};
    bool doWriteSeparator{ true };
    if (conf.GetSaveFile() == "none")
    {
        fileName = conf.GetSavePath() == "default" ? CLI::narrow(GetServiceFolder(logger)) : conf.GetSavePath();
        if (fileName == "")
        {
            utf8Strings.reserve(bytesVec.size());
            logger->error("unable get the service location, returning");
            return;
        }
        fileName += "\\output.";
        fileName += conf.GetExtension();
    }
    else
    {
        fileName = conf.GetSaveFile();
        std::string prevFileCont{ StringFromFile(fileName,logger) };
        if (conf.GetExtension() == "csv")
        {
            utf8Strings.reserve(bytesVec.size());
            if (!prevFileCont.empty())
            {
                ss << prevFileCont;
                doWriteSeparator = false;                               //we don't need to write one another separator to the csv file
            }
        }
        else
        {
            utf8Strings.push_back(prevFileCont);
            utf8Strings.reserve(bytesVec.size() + 1);
        }
    }
    try
    {
        for (const auto& bytes : bytesVec)
            utf8Strings.push_back(CLI::narrow(BytesToWide(bytes)));
    }
    catch (const std::exception& exc)
    {
        std::string msg{ "error encountered: " };
        msg += exc.what();
        msg += ", exiting";
        logger->error(msg);
        return;
    }
    if (conf.GetExtension() == "xml")
    {
        if (!XMLToStream(utf8Strings, ss, logger))
        {
            logger->error("XMLToStream is failed, SaveTable exiting");
            return;
        }
    }
    else if (conf.GetExtension() == "json")
    {
        if (!JSONToStream(utf8Strings, ss, logger))
        {
            logger->error("JSONToStream is failed, SaveTable exiting");
            return;
        }
    }
    else if (conf.GetExtension() == "csv")
    {
        if (!JSONToCSVStream(utf8Strings, ss, logger, doWriteSeparator))
        {
            logger->error("JSONToStream is failed, SaveTable exiting");
            return;
        }
    }
    else
    {
        logger->error("Unexpected file extension encountered, exiting");
        return;
    }
    SaveToFile(fileName, CLI::widen(ss.str()), logger);
    conf.SetSaveFile(fileName);
    logger->trace("SaveTableSingleMode ended successfully");
}
std::wstring BytesToWide(const std::string& bytes)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, bytes.c_str(), -1, nullptr, 0);
    if (size_needed <= 0) {
        // Handle error if needed
        return std::wstring();
    }
    std::wstring wstrTo(size_needed - 1, 0);  // size_needed includes null terminator, we don't need it in std::wstring
    MultiByteToWideChar(CP_UTF8, 0, bytes.c_str(), -1, &wstrTo[0], size_needed);
    return wstrTo;
}
bool XMLToStream(std::vector<std::string>& utf8Strings, std::stringstream& sStreamToWrite, const Logger& logger)
{
    logger->trace("XMLToStream entry: ");
    auto doc{ std::make_unique<rapidxml::xml_document<>>() };
    auto anotherDoc{ std::make_unique<rapidxml::xml_document<>>() };
    auto& conf{ ProgramConfiguration::GetInstance() };
    logger->trace("Trying to parse, reduce and merge xmls");
    try
    {
        utf8Strings[0].append("\0");
        doc->parse<0>(&utf8Strings[0][0]);
        if (!ReduceXML(*doc.get(), conf.GetReqParams(), logger))
        {
            logger->error("unable to save reduce the first currency");
            return false;
        }
        for (int iii{ 1 }; iii < static_cast<int>(utf8Strings.size()); ++iii)
        {
            utf8Strings[iii].append("\0");
            anotherDoc->parse<0>(&utf8Strings[iii][0]);
            if (!ReduceXML(*anotherDoc.get(), conf.GetReqParams(), logger))
            {
                std::string msg{ "enable to delete the " };
                msg += std::to_string(iii);
                msg += " we will continue";
                logger->error(msg);
                continue;
            }
            logger->trace("trying to merge");
            auto nodeToCopy{ anotherDoc->first_node("exchange")->first_node("currency") };
            if (!nodeToCopy)
            {
                logger->error("something went wrong when copy");
                return false;
            }
            auto copy{ doc->clone_node(nodeToCopy) };
            doc->first_node("exchange")->append_node(copy);
        }
        logger->trace("Parsing, reducing and merging was successfully performed");
        rapidxml::print<char>(sStreamToWrite, *doc.get(), 0);
    }
    catch (const std::exception& exc)
    {
        logger->error(exc.what());
        return false;
    }
    logger->trace("XMLToStream successfully ended");
    return true;
}
bool ReduceXML(rapidxml::xml_document<>& doc, const std::vector<std::string>& paramsToSave, const Logger& logger)
{
    logger->trace("Reduce XML entry");
    rapidxml::xml_node<>* root{ doc.first_node("exchange") };
    if (!root)
        return false;
    logger->trace("root was found");
    rapidxml::xml_node<>* node{ root->first_node("currency") };
    if (!node)
        return false;
    logger->trace("currency node was found, starting reduce");
    for (auto* subNode{ node->first_node() }; subNode;)
    {
        const std::string& subNodeName{ subNode->name() };
        if (std::find(paramsToSave.begin(), paramsToSave.end(), subNodeName) == paramsToSave.end())
        {
            auto* nextSubNode{ subNode->next_sibling() };
            node->remove_node(subNode);
            subNode = nextSubNode;
            continue;
        }
        else
            subNode = subNode->next_sibling();
    }
    logger->trace("file was reduced, exiting");
    return true;
}
bool JSONToStream(const std::vector<std::string>& utf8Strings, std::stringstream& sStreamToWrite, const Logger& logger)
{
    logger->trace("JSONToStream entry");
    auto& conf{ ProgramConfiguration::GetInstance() };
    try
    {
        logger->trace("trying to find keys to remove");
        std::vector<std::string> keysToDelete{};
        keysToDelete.reserve(gAllowedRequestParams.size());
        nlohmann::json json{ nlohmann::json::parse(utf8Strings[0]) };
        nlohmann::json anotherJson{};
        logger->info("keys to delete: ");
        const auto& items{ json.front().items() };

        for (const auto& item : items)
            if (!IsSubset({ item.key() }, conf.GetReqParams() ))
            {
                keysToDelete.push_back(item.key());
                logger->info(item.key());
            }

        for (const auto& key : keysToDelete)
            json.begin()->erase(key);

        for (int iii{ 1 }; iii < static_cast<int>(utf8Strings.size()); ++iii)
        {
            anotherJson = nlohmann::json::parse(utf8Strings[iii]);
            const auto& items{ anotherJson.front().items() };
            for (const auto& item : items)
                if (!IsSubset({ item.key() }, conf.GetReqParams()))
                {
                    keysToDelete.push_back(item.key());
                    logger->info(item.key());
                }
            for (const auto& key : keysToDelete)
                json.begin()->erase(key);
            for (const auto& key : keysToDelete)
                anotherJson.begin()->erase(key);
            json.push_back(*anotherJson.begin());
        }

        sStreamToWrite << json.dump(4);
    }
    catch (const std::exception& exc)
    {
        logger->info("error encountered: ");
        logger->error(exc.what());
        logger->trace("return");
        return false;
    }
    logger->trace("JSONToStream ended successfully");
    return true;
}
bool JSONToCSVStream(const std::vector<std::string>& utf8Strings, std::stringstream& sStreamToWrite, const Logger& logger, bool doWriteSeparator)
{
    logger->trace("JSONToCSV entry");
    try
    {
        char separator{ ';' };
        if(doWriteSeparator) 
            sStreamToWrite << "sep =" << separator << "\r\n";
        auto& conf(ProgramConfiguration::GetInstance());
        nlohmann::json json{};
        auto requestParams{ conf.GetReqParams() };

        auto txtReqIt{ std::find(requestParams.begin(), requestParams.end(), std::string{ "txt" }) };

        if (txtReqIt != requestParams.end())
            requestParams.erase(txtReqIt);                  //it will work badly at least at MS Excel

        for (int iii{ 0 }; iii < static_cast<int>(conf.GetCurrs().size()); ++iii)
        {
            sStreamToWrite << conf.GetCurrs()[iii];
            for (int iii{ 1 }; iii < static_cast<int>(requestParams.size()); ++iii)
                sStreamToWrite << separator;
            sStreamToWrite << "\r\n";
            json = nlohmann::json::parse(utf8Strings[iii]);

            for (int iii{ 0 }; iii < static_cast<int>(requestParams.size()) - 1; ++iii)
                sStreamToWrite << requestParams[iii] << separator;
            sStreamToWrite << requestParams.back() << "\r\n";

            for (int iii{ 0 }; iii < static_cast<int>(requestParams.size()) - 1; ++iii)
                sStreamToWrite << json.front().at(requestParams[iii]) << separator;
            sStreamToWrite << json.front().at(requestParams.back()) << "\r\n";
        }
    }
    catch (const std::exception& exc)
    {
        logger->error("error ecountered: ");
        logger->error(exc.what());
        return false;
    }
    return true;
}
bool SaveToFile(const std::string& fileName, const std::wstring& dataToSave, const Logger& logger)
{
    logger->trace("SaveToFile entry");
    std::wofstream outfile(fileName, std::ios::binary);

    if (outfile.is_open())
    {
        outfile.imbue(std::locale("uk_UA.UTF-8"));
        outfile << dataToSave.c_str();
        outfile.close();
        logger->trace("SaveToFile ended successfully");
        return true;
    }
    else
    {
        logger->error("SaveToFile failure");
        return false;
    }
}
bool SvcInstall(const Logger& logger)
{
    logger->trace("Starting installation");

    ProgramConfiguration& conf{ ProgramConfiguration::GetInstance() };
    if (conf.GetConfigFilePath() != "default")
    {
        logger->trace("Writing custom configuration file: ");
        logger->info(conf.GetConfigFilePath());

        if (GetFileExtension(conf.GetConfigFilePath()) != "ini")
        {
            std::cerr << "wrong custom configuration file extension\n";
            logger->error("wrong custon configuration file extension, returning");
            return false;
        }
    }
    logger->trace("trying to create configuration file(s)");
    if (!conf.SaveConfiguration(logger))
    {
        std::cerr << "Unable to create configuration file(s)\n";
        logger->error("Unable to create configuration file(s), returning");
        return false;
    }
    logger->trace("created successfully");

    TCHAR unquotedPath[MAX_PATH];
    if (!GetModuleFileName(nullptr, unquotedPath, MAX_PATH))
    {
        std::wcerr << L"Cannot get path to the service execution file: " << GetLastError() << "\n";
        logger->error(std::string{ "Cannot get path to the service execution file: " } + std::to_string(GetLastError()));
        logger->info(std::string{ "unquotedPath: " } + CLI::narrow(unquotedPath));
        return false;
    }

    TCHAR path[MAX_PATH];
    StringCbPrintf(path, MAX_PATH, TEXT("\"%s\""), unquotedPath);
    logger->info(std::string{ "Path to the main app: " } + CLI::narrow(path));

    // SCM handling
    SC_HANDLE schSCManager{ OpenSCManager(nullptr,					// local PC
                                          nullptr,					// default database
                                          SC_MANAGER_ALL_ACCESS) };
    if (!schSCManager)
    {
        std::wcerr << L"OpenSCManager failed: " << GetLastError() << "\n";
        logger->error(std::string{ "OpenSCManager failed: " } + std::to_string(GetLastError()));
        return false;
    }
    // Service creation
    SC_HANDLE schService{ CreateService(schSCManager,				// SCM database 
                                        SVCNAME,					// name of service 
                                        SVCNAME,					// service name to display 
                                        SERVICE_ALL_ACCESS,			// desired access 
                                        SERVICE_WIN32_OWN_PROCESS,	// service type 
                                        SERVICE_DEMAND_START,		// start type 
                                        SERVICE_ERROR_NORMAL,		// error control type 
                                        path,						// path to service's binary 
                                        nullptr,					// no load ordering group 
                                        nullptr,					// no tag identifier 
                                        nullptr,					// no dependencies 
                                        nullptr,					// LocalSystem account 
                                        nullptr) };					// no password 

    if (!schService)
    {
        std::wcerr << L"CreateService failed: " << GetLastError() << "\n";
        logger->error(std::string{ "CreateService failed: " } + std::to_string(GetLastError()));
        CloseServiceHandle(schSCManager);
        return false;
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    if (conf.GetControlState() == ControlState::Start)
        if (StartSvc(logger))
            std::cout << "Service was started\n";
        else
            std::cerr << "Can't start the service\n";
    logger->trace("SvcInstall exiting");
    return true;
}
bool StartSvc(const Logger& logger)
{
    logger->trace("StartSvc entry");
    SC_HANDLE schSCManager{ OpenSCManager(nullptr,				// local PC
                                          nullptr,				// default database
                                          SC_MANAGER_CONNECT) };
    if (!schSCManager)
    {
        logger->error(std::string{ "OpenSCManager failed: " } + std::to_string(GetLastError()));
        return false;
    }
    SC_HANDLE schService{ OpenService(schSCManager, SVCNAME, SERVICE_START) };
    if (!schService)
    {
        logger->error(std::string{ "OpenService failed: " } + std::to_string(GetLastError()));
        CloseServiceHandle(schSCManager);
        return false;
    }
    if (!StartService(schService, 0, nullptr))
    {
        logger->error(std::string{ "Start service failed: " } + std::to_string(GetLastError()));
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }
    logger->trace("Service started succesfully, ending");
    return true;
}
bool SvcUninstall(const Logger& logger)
{
    logger->trace("Starting uninstallation");
    if (!IsStopped(logger))
    {
        if (!StopSvc(logger))
        {
            std::wcerr << "Something went wrong when stopping service, maybe you need to manually restart the PC after uninstallation\n";
            logger->error("Something went wrong when stopping service");
        }
    }
    SC_HANDLE schSCManager{ OpenSCManager(nullptr,					// local PC
                                          nullptr,					// default database 
                                          SC_MANAGER_CONNECT) };
    if (!schSCManager)
    {
        logger->error(std::string{ "OpenSCManager failed when uninstalling: : " } + std::to_string(GetLastError()));
        std::wcerr << L"OpenSCManager failed: " << GetLastError() << "\n";
        return false;
    }
    SC_HANDLE schService{ OpenService(schSCManager, SVCNAME, DELETE) };
    if (!schService)
    {
        logger->error(std::string{ "OpenService failed when uninstalling: : " } + std::to_string(GetLastError()));
        std::wcerr << L"OpenService failed: " << GetLastError() << "\n";
        CloseServiceHandle(schSCManager);
        return false;
    }
    if (!DeleteService(schService))
    {
        logger->error(std::string{ "DeleteService failed when uninstalling: : " } + std::to_string(GetLastError()));
        std::wcerr << L"DeleteService failed: " << GetLastError() << "\n";
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    ProgramConfiguration& conf{ ProgramConfiguration::GetInstance() };
    bool doDeleteSvcFolder{ !conf.DoSaveSvcFolderQ() };
    logger->info(std::string{ "doDeleteSvcFolder = " } + std::to_string(doDeleteSvcFolder));
    if (doDeleteSvcFolder)
    {
        std::wstring svcDir{ GetServiceFolder(logger) };
        if (DeleteDirectory(svcDir, logger))
        {
            std::wcout << "Service directory successfully deleted\n";
            logger->trace("Service directory successfully deleted!");
        }
        else
        {
            std::wcerr << "Unable to delete the SvcDirectiry, probably you need to do that by hands\n";
            logger->error(std::string{ "Unable to delete the service directory: " } + CLI::narrow(svcDir));
        }
    }
    logger->trace("Uninstallation is successful, exiting SvcUninstall");
    return true;
}
bool StopSvc(const Logger& logger)
{
    if (!gSvcStatusHandle)
    {
        logger->trace("Stopping funtion entry");
        SC_HANDLE schSCManager{ OpenSCManager(nullptr,				// local PC
                                              nullptr,				// default database
                                              SC_MANAGER_CONNECT) };
        if (!schSCManager)
        {
            logger->error(std::string{ "OpenService failed when stoppping: " } + std::to_string(GetLastError()));
            return false;
        }

        SC_HANDLE schService{ OpenService(schSCManager, SVCNAME, SERVICE_STOP) };
        if (!schService)
        {
            logger->error(std::string{ "OpenService failed when stoppping for uninstall: " } + std::to_string(GetLastError()));
            CloseServiceHandle(schSCManager);
            return false;
        }
        if (ControlService(schService, SERVICE_CONTROL_STOP, &gSvcStatus))
        {
            logger->trace("Service sucessfully stopped");
            CloseServiceHandle(schService);
            CloseServiceHandle(schSCManager);
            return true;
        }
    }
    return false;
}
bool SvcControl(const Logger& logger)
{
    logger->trace("SvcControl entry");
    logger->trace("moving setted configuration from console to temp vars");
    ProgramConfiguration& conf{ ProgramConfiguration::GetInstance() };
    int tempFetchRate{ conf.GetFetchRate() };
    std::vector<std::string> tempCurrs{ conf.GetCurrs() };
    std::vector<std::string> tempReqParams{ conf.GetReqParams() };
    std::string tempConfigFilePath{ conf.GetConfigFilePath() };
    if(tempConfigFilePath != "default")
        if (GetFileExtension(tempConfigFilePath) != "ini")
        {
            std::cerr << "wrong configuration file extension, it won't switched\n";
            tempConfigFilePath = "default";
        }
    std::string tempExtension{ conf.GetExtension() };
    std::string tempSavePath{ conf.GetSavePath() };
    SaveMode tempSaveMode{ conf.GetSaveMode() };
    std::string tempSaveFile{ conf.GetSaveFile() };

    bool wasRunning{ IsRunning(logger) };
    logger->trace("stopping the service");
    if (!IsStopped(logger))
    {
        if (!StopSvc(logger))
        {
            logger->error("unable to stop the service");
            return false;
        }
        else
            std::cout << "service stopped\n";
    }
    logger->trace("validating configuraton");
    if (!conf.ValidateConfigurationFile(logger))
    {
        std::cerr << "invaid configuration files(s), we will fix them\n";
        logger->error("invaid configuration files(s), we will fix them");
        if (!conf.FixConfigurations(logger))
        {
            std::cerr << "unable to fix configuration files(s)\n";
            logger->error("unable to fix configuration files(s)");
            return false;
        }
    }
    logger->trace("setting from configuration to memory");
    if (!conf.SetFromConfiguration(logger))
    {
        logger->error("can't set configuration from file(s)");
        return false;
    }
    logger->trace("setting console entered options to the memory");

    const auto& settedOptions{ conf.GetSettedOptions() };
    if (settedOptions.find("fetch_rate") != settedOptions.end())
        conf.SetFetchRate(tempFetchRate);
    if (settedOptions.find("currencies") != settedOptions.end())
        conf.SetCurrs(tempCurrs);
    if (settedOptions.find("request_parameters") != settedOptions.end())
        conf.SetReqParams(tempReqParams);
    if (settedOptions.find("configuration_file") != settedOptions.end())
        conf.SetConfigFilePath(tempConfigFilePath);
    if (settedOptions.find("extension") != settedOptions.end())
        conf.SetExtension(tempExtension);
    if (settedOptions.find("save_path") != settedOptions.end())
        conf.SetSavePath(tempSavePath);
    if (settedOptions.find("save_mode") != settedOptions.end())
        conf.SetSaveMode(tempSaveMode);
    if (settedOptions.find("save_file") != settedOptions.end())
        conf.SetSaveFile(tempSaveFile);


    if (!conf.SaveConfiguration(logger))
    {
        logger->error("unable to save configuration from control");
        return false;
    }




    if (conf.GetControlState() == ControlState::Stop)
        return true;
    else if (conf.GetControlState() == ControlState::Start || wasRunning)
    {
        if (!StartSvc(logger))
            return false;
        std::cout << "service started\n";
    }
    return true;
}
SERVICE_STATUS_PROCESS GetServiceStatus()
{
    SC_HANDLE schSCManager{ OpenSCManager(nullptr,				// local PC
                                          nullptr,				// default database
                                          SC_MANAGER_CONNECT) };
    if (!schSCManager)
    {
        std::string msg{ "OpenSCManager failed: " };
        msg += std::to_string(GetLastError());
        throw std::exception{ msg.c_str() };
    }
    SC_HANDLE schService{ OpenService(schSCManager, SVCNAME, SERVICE_QUERY_STATUS) };
    if (!schService)
    {
        CloseServiceHandle(schSCManager);
        std::string msg{ "OpenService failed: " };
        msg += std::to_string(GetLastError());
        throw std::exception{ msg.c_str() };
    }
    SERVICE_STATUS_PROCESS ssp{};
    DWORD bytesNeeded{};
    if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) 
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        std::string msg{ "QueryServiceStatusEx failed: " };
        msg += std::to_string(GetLastError());
        throw std::exception{ msg.c_str() };
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return ssp;
}
bool IsRunning(const Logger& logger)
{
    try 
    {
        SERVICE_STATUS_PROCESS ssp{ GetServiceStatus() };
        return ssp.dwCurrentState == SERVICE_RUNNING;
    }
    catch (const std::exception& exc) 
    {
        logger->error("Error encountered when IsRunning check: ");
        logger->error(exc.what());
        return false;
    }
}
bool IsStopped(const Logger& logger)
{
    try
    {
        SERVICE_STATUS_PROCESS ssp{ GetServiceStatus() };
        return ssp.dwCurrentState == SERVICE_STOPPED;
    }
        catch (const std::exception& exc)
    {
        logger->error("Error encountered when IsRunning check: ");
        logger->error(exc.what());
        return false;
    }
}
bool DeleteDirectory(const std::wstring& directoryPath, const Logger& logger)
{
    logger->trace("Entry the DeleteDirectory");
    WIN32_FIND_DATAW findData;
    HANDLE hFind{ FindFirstFileW((directoryPath + L"\\*").c_str(), &findData) };
    if (hFind == INVALID_HANDLE_VALUE)
    {
        if (!RemoveDirectoryW(directoryPath.c_str()))
        {
            logger->error(std::string{ "Unable to delete the service directory: " } + std::to_string(GetLastError()));
            logger->trace("Exiting the DeleteDirectory");
            return false;
        }
    }
    std::wstring filePath;
    filePath.reserve(MAX_PATH);
    do
    {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0)
        {
            filePath = directoryPath + L"\\" + findData.cFileName;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                DeleteDirectory(filePath, logger);
            else
                DeleteFileW(filePath.c_str());
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);

    if (!RemoveDirectoryW(directoryPath.c_str()))
    {
        logger->error(std::string{ "Unable to delete the service directory: " } + std::to_string(GetLastError()));
        logger->trace("Exiting the DeleteDirectory");
        return false;
    }

    return true;
}
std::string StringFromFile(const std::string& fileName, const Logger& logger)
{
    logger->trace("StringFromFile entry");
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        logger->error("unable to open the file");
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    logger->trace("StringFromFile outry");
    return buffer.str();
}


int main(int argc, char** argv)
{
    Logger logger{ LoggerInit("","ConsoleLog.log","main",gMaxLoggerFileSize)};
    if (!logger)
    {
        std::cerr << "unable to get the log file, exiting";
        return 1;
    }
    logger->set_level(spdlog::level::trace);
    logger->trace("Start logging, main entry");
    if (argc == 1)                               //probably SCM entry
    {
        logger->info("starting with argc = 1, argv: ");
        logger->info(argv[0]);
        if(!IsSvcInstalled(logger))
        {
            logger->error("Service wasn't installed");
            std::cerr << "Service isn't installed, try to properly install\n";
            return 0;
        }

        LPWSTR scvName{ const_cast<LPWSTR>(SVCNAME) };
        SERVICE_TABLE_ENTRY svcEntry{
            scvName,
            (LPSERVICE_MAIN_FUNCTION)SvcMain };

        logger->trace("SvcMain was registered");
        if (!StartServiceCtrlDispatcher(&svcEntry))
        {
            std::wcerr << "Failed to start the control dispatcher, exiting";
            logger->error("ServiceCtrlDispatcher failed");
            LoggerEnd(logger);
            return 1;
        }
        logger->trace("main ends");
        LoggerEnd(logger);
        return 0;
    }

    ProgramConfiguration& conf{ ProgramConfiguration::GetInstance() };
    conf.SetFromConsole(argc, argv, logger);
    switch (conf.GetConsoleState())
    {
    case ConsoleState::Default:
    case ConsoleState::Unexpected:
        std::cerr << "unexpected, shutdown";
        logger->error("unexpected console path, return");
        LoggerEnd(logger);
        return 1;
    case ConsoleState::Install:
        if (IsServiceInstalled(logger))
        {
            std::cerr << "Service is already installed\n";
            logger->info("service is already installed");
            break;
        }
        if (!SvcInstall(logger))
        {
            std::cerr << "Service installation failed\n";
            logger->error("Service installation failed");
        }
        else
        {
            std::cout << "Service installed successfully\n";
            logger->info("Service installed successfully");
        }
        break;
    case ConsoleState::Uninstall:
        if (!IsServiceInstalled(logger))
        {
            std::cerr << "Service even isn't installed\n";
            logger->info("Service isn't installed");
            break;
        }
        if (!SvcUninstall(logger))
        {
            std::cerr << "Service uninstallation failed\n";
            logger->error("Service uninstallation failed");
        }
        else
        {
            std::cout << "Service uninstalled successfully\n";
            logger->info("Service uninstalled successfully");
        }
        break;
    case ConsoleState::Control:
        if (!IsServiceInstalled(logger))
        {
            std::cerr << "Service even isn't installed\n";
            logger->info("Service isn't installed");
            break;
        }
        if (SvcControl(logger))
            std::cout << "control was successfully performed\n";
        else
            std::cerr << "something went wrong";
        break;
    }

    logger->trace("main ends");
    LoggerEnd(logger);
    return 0;
}