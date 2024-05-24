# TestSvc

TestSvc fetches currency rates from the NBU (National Bank of Ukraine) API and saves the data to file. The service supports control under both installation/uninstallation and control under services execution from console. TestSvc supports saving currencies rates to a single specified file or creating a new file for each request to NBU API based on specified mode and file extension. Fetch rate, currencies, request parameters, mode and file extension can be setted both from console and configuration.


## Key features:

- **Fetch currency rates from NBU API: Automatically retrieve the latest currency exchange rates**.
- **Flexible saving options: Save the rates to a single file or create a new file for each request**.
- **Configurable request frequency: Set how often the service should fetch new rates**.
- **Customizable file extensions: Specify the desired file extension for saved data (XML, JSON or CSV)**.
- **Console and configuration file support: Set fetch rate, currencies, request parameters, mode, and file extension from the console or configuration file.**
- **Service control: Install/uninstall and control service execution from the console.**
- **Logging: Service uses logging into the log files, which saved in the execution file path(for installation/uninstallation and control requests logging) and service folder path in program files x86 (for service exetion logging).**

## Table of Contents

- [Dependencies](#dependencies)
- [Installation](#installation)
- [Usage](#usage)
- [Execution modes and options](#execution-modes-and-options)
- [Configuration](#configuration)
- [Uninstallation](#uninstallation)
- [Logging](#logging)
- [License](#license)
- [Contact](#contact)

## Dependencies

Service depends on:

- **CLI11:** for easy comand line parsing and using.
- **CURLlib:** for URL requests.
- **RapidXML:** for XML parsing, modifying and saving.
- **nlohmann/json:** for JSON parsing, modifying and saving.

All used libraries are header-only (RapidXML, nlohmann/json, CLI11) or static with headers (CURL), so it doesn't have any runtime dependencies.  

## Installation:

1. After compilation, you need to put the execution file into the directory *without cyrillic symbols and run command line interface as administrator (with administrator permissions).
2. You can installation procedure using:
```console
TestScvApp.exe -i
```
Which will install service (if it isn't installed yet) to the SCM, will set default configuration (see the [Configuration](#configuration) section) setting and won't start service. For starting service after installation, you can use:
```console
TestScvApp.exe -i --start
```
where you'll use allowed control option for installation. There are more allowed when install options (fetch_rate, currencies, request_parameters, exstension, configuration_file, save_path, file_to_save and so on with their short forms) which you can see using:
```console
TestScvApp.exe -h
```
They will be discussed later at the chapters [Usage](#usage) and [Execution modes and options](#execution-modes-and-options).

## Usage
When installed without using any specifications like:
```console
TestScvApp.exe -i
```
or
```console
TestScvApp.exe -i --start
```
Service will installed (and optionally started with default [Configuration](#configuration)). However, you can specify specify additional options during installation, which you can get using:
```console
TestScvApp.exe -h
```
or
```console
TestScvApp.exe --help
```
It will show the help message, where you can see commands which you can use (with their long and short form (if allowed) like so:
```console
Options:
  -h,--help                   Print this help message and exit
  -i,--install Excludes: --uninstall --control --stop --save_svc_dir
                              To install the service
  -u,--uninstall Excludes: --install --control --start --stop --different_files --single_file --none_file --fetch_rate --currency --extension --request_parameters --configuration_file --save_path --file_to_save
                              To uninstall the service
  --control Excludes: --install --uninstall --save_svc_dir
                              To control the service
  --start Excludes: --uninstall --stop
                              To control the service
  --stop Excludes: --install --uninstall --start
                              To control the service
  --different_files Excludes: --uninstall --single_file --file_to_save
                              To set the different files save mode
  --single_file Excludes: --uninstall --different_files
                              To set the single file save mode
  --none_file Excludes: --uninstall --file_to_save
                              To set the save file for single_file to none
  -r,--fetch_rate INT:INT in [1 - 2480] Excludes: --uninstall
                              To set fetch rate
  -v,--currency TEXT:{AUD,CAD,CNY,HRK,CZK,DKK,HKD,HUF,INR,IDR,ILS,JPY,KZT,KRW,MXN,MDL,NZD,NOK,SAR,SGD,ZAR,SEK,CHF,EGP,GBP,USD,BYN,RON,TRY,XDR,BGN,EUR,PLN,DZD,BDT,AMD,IRR,IQD,KGS,LBP,LYD,MYR,MAD,PKR,VND,THB,AED,TND,UZS,TMT,RSD,AZN,TJS,GEL,BRL,XAU,XAG,XPT,XPD} ... Excludes: --uninstall
                              To set vals to save
  -e,--extension TEXT:{xml,json,csv} Excludes: --uninstall
                              To set extension of file to save
  -p,--request_parameters TEXT:{exchangedate,r030,cc,txt,enname,rate,units,rate_per_unit,group,calcdate} ... Excludes: --uninstall
                              To set params to save
  -c,--configuration_file TEXT:FILE Excludes: --uninstall
                              To set the config path for controll or uninstallation
  -s,--save_path TEXT:PATH(existing) Excludes: --uninstall
                              To set path where files will be saved in different files mode
  --save_svc_dir Excludes: --install --control
                              Remove the svc folder or not for uninstallation
  -f,--file_to_save TEXT:FILE Excludes: --uninstall --different_files --none_file
                              To specify file to save (append) table
```
with desctiptions and exclusions to each command.

Furthermore, when service is working, you can use:
```console
TestScvApp.exe --control --start
```
to start the service, 
```console
TestScvApp.exe --control --stop
```
to stop the service, or even
```console
TestScvApp.exe --control --start -r 10 -v AUD, CAD, EUR, USD -p txt, rate, cc -c "C:\TestDir\config.ini" -f "C:\TestDir\save\24-05-2024 11_51_output.csv" -e csv --single_file
```
To choose control mode of console run (which allows controlling the service when it is stopped or running, but installed), start the service, set fetch_rate to 10, set currencies to save (AUD, CAD, EUR, USD), parapeters to save, place of custom configuration file and existing file to save with extension to save.
**Service will rember it's options or configuration, so will update only proposed options, if they are correct**

**WARNING:** When you choose the files or directories to save and custom config file, you need to use existing path (or well-formated file to store data, or config file). Use double quotes to ensure all spaces will threaded correctly. 
**WARNING:** Proper separator for arrays of values (if you need to fetch several currencies with several parameters) is a comma (,).
**WARNING:** Service will share the same parameters for each currency, which wil be set and will save it into same file.

## Execution modes and options

Service supports two working modes: single_file (flag to set the mode via console control) and different_files (flag to set the mode via console control).
### Different files mode
 - **different_files** is the default mode, will save data to different files on each service's request to the NBU API (which are'll be name like "dd-mm-yyyy hh-mm_otput" with specified exstension into specified location (and with default to default if there isn't).
### Single file mode
 - **single_file** mode will update the data to specified well-formated file with specified exstension (you need to specify both file_name like `-f "C:\TestDir\save\24-05-2024 11_51_output.csv"` and extension like `-e csv` if you change the extension for it's propper work. If there isn't any specified file, it will save to the default "output" file with specified extension (or default if not) to the save_path if existing or default (if not) locations.

### More about options
#### Fetch Rate
 - **fetch_rate** sets, how many times per service will request to NBU API (with default value 1440, means one time per minute). Can be setted wheter during installation:
```console
TestScvApp.exe --install -r 10
```
or during control execution:
```console
TestScvApp.exe --control -r 10
```
#### Currencies
 - **currencies**: which currencies you are instered in (with default EUR and USD), you can set it both using control logic and during installation:
```console
TestScvApp.exe --install -r 10 --currencies CAD, DKK, HKD
```
or during cotrol execution:
```console
TestScvApp.exe --control -r 10 -v CAD, DKK, HKD
```
*Here you can see that you can short-form or long-form weys to specify option (but not both at the same time!)*.
#### Request Parameters
You can retrive whole data (from NBU) for considered currency from NBU:
```xml
<exchange>
<currency>
<exchangedate>27.05.2024</exchangedate>
<r030>36</r030>
<cc>AUD</cc>
<txt>Австралійський долар</txt>
<enname>Australian Dollar</enname>
<rate>26.5323</rate>
<units>1</units>
<rate_per_unit>26.5323</rate_per_unit>
<group>1</group>
<calcdate>24.05.2024</calcdate>
</currency>
</exchange>
```
But probably you don't want all of this, so you need to specify, with this purpose you can use
 - **request_parameters** options, which allowed both from `--control` and `--install`. Here you can specify, wich parameters you need as:
```console
TestScvApp.exe --control -p txt, calcdate, enname, exchangedate, rate
```
Default parameters are: exchangedate, cc, rate.
#### Configuration file
 - **configuration_file**: if you would like to use your own configuration file, you can you this option to specify your own config file path (during installation or --control) like:
```console
TestScvApp.exe --control -c "C:\TestDir\config.ini"
```
#### Extension
 - extension: with this parameter you will specify the file format to save (default is json) like:
```console
TestScvApp.exe --control --different_files -e csv
```
or
```console
TestScvApp.exe --control --single_file  -e xml -f "C:\TestDir\save\24-05-2024 11_51_output.xml"
```
#### Save Path
 - **save_path** - parameter to specify, in which directory service with save files in `differennt_files` mode and `single_file` mode (if you didn't specify file to save or it was unpropper) with default path to save - service's progran files x86 folder (TestSvc).
#### File To Save
 - **file_to_save** - options that allows you to specify (**well-foramtted**) file to appending currency rates to (with default - `output`.extension within the service's ProgramFiles x86 (TestSvc) folder or within the save_path, if specified). if you need to set the value by default, you should use `--none_file` flag.

## Configuration

Service uses the ".ini" format to store the configuration. The default configuratoin file is named `config.ini` and is saved at the service's Program Files (x86) folder (TestSvc).
Every time, before making request, service will refer to configuration file to retrive request and save parameters. You can specify your own config file during installation (`-i`, `--install`) or control (`--control`) using `-c Your\Program\path.ini`:
```console
TestScvApp.exe -i -c "C:\TestDir\config.ini"
```
Your configuration should look like this:
```ini
fetch_rate = 720
currencies = EUR, USD, AUD, CAD
request_parameters = txt, units, exchangedate
configuration_file = "C:\\TestDir\\config.ini"
save_path = "C:\\TestDir\\save"
save_mode = different_files
save_file = "C:\\TestDir\\save\\output.json"
extension = json
```
### Key points:
- **Control options precedence:** control options provided via console will owerride the settings in your configuration file, configuration file that you provide will be updated using console-provided values.
- **Configuration file is considered as propper:** at least if it can parsed as ".ini-formated file" and all specified options innit are correct. You can even provide the empty .ini file (if it existing) and application will update it with current state from memory (or will add missing parameters).
- **Path to your custom configuration file** is stored at the service's default configuration file located ath the `ProgramFiles x86\TestSvc`. Both files will update equally: if you will change your custom file, configuration will updated at the default config file as well.
- **Error Handling:** If the custom config file contains unexpected parameters, the application will try to read and save the allowed values of options, then rewrite your config file to update incorrect values with the current ones.
- **File Deletion:** If you delete the custom config file, the service will switch to the default one, if it exists, and use the saved options. If there is no default config file, it will create one and update it with the current configuration from memory. If you delete the default config file, the service will forget about your custom file and create a new default one.

## Uninstallation

Uninstallation pocedure is easy to perform using console:
```console
TestScvApp.exe -u
```
or if you want to save the TestSvc directory in Program Files (x86), you must add corresponding flag:
```console
TestScvApp.exe -u --save_svc_dir
```

## Logging

Logging is handled using the header-only spdlog library, chosen for its speed, lightweight nature, header-only implementation, and user-friendly license. The application utilizes two distinct logging paths:

1. Сonsole-related activities are logged to the file within service's `.exe` file's folder.
2. Log files with the execution of the `SvcMain` function and the operation of the `workThread` function are in the \ProgramFiles (x86)\TestSvc directory. 

`SvcMain` is the service main function registered by `main`, responsible for initiating all other operations. 

`WorkThread` is a function that performs the primary tasks of the service and runs on a separate thread.

## License
-----

## Contact
Zabora Daniil
- e-mail [zaboradaniil@gmail.com]
