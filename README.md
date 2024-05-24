# TestSvc

TestSvc fetches currency rates from the NBU (National Bank of Ukraine) API and saves the data to file. The service supports control under both installation/uninstallation and control under services execution from console. TestSvc supports saving currencies rates to a single specified file or creating a new file for each request to NBU API based on specified mode and file extension. Fetch rate, currencies, request parameters, mode and file extension can be setted both from console and configuration.


## Key features:

- *Fetch currency rates from NBU API: Automatically retrieve the latest currency exchange rates.
- *Flexible saving options: Save the rates to a single file or create a new file for each request.
- *Configurable request frequency: Set how often the service should fetch new rates.
- *Customizable file extensions: Specify the desired file extension for saved data (XML, JSON or CSV).
- *Console and configuration file support: Set fetch rate, currencies, request parameters, mode, and file extension from the console or configuration file.
- *Service control: Install/uninstall and control service execution from the console.
- *Logging: Service uses logging into the log files, which saved in the execution file path(for installation/uninstallation and control requests logging) and service folder path in program files x86 (for service exetion logging)

## Table of Contents

- [Dependencies](#dependencies)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Uninstallation](#uninstallation)
- [License](#license)
- [Contact](#contact)

## Dependencies

Service depends on:

- *CLI11: for easy comand line parsing and using.
- *CURLlib: for URL requests.
- *RapidXML: for XML parsing, modifying and saving.
- *nlohmann/json: for JSON parsing, modifying and saving.

All used libraries are header-only (RapidXML, nlohmann/json, CLI11) or static with headers (CURL), so it doesn't have any runtime dependencies.  

## Installation:

1. After compilation, you need to put the execution file into the directory without cyrillic symbols and run command line interface as administrator (with administrator permissions).
2. You can run 
