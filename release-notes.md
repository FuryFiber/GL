# Release notes
### New modules

### Bugfixes

# Install
## Linux x64
### Using package
1. Download `GL-0.0.2-alpha-lin-x64.vcvplugin`
2. Copy the file into `~/.local/share/Rack2/plugins-lin-x64`
3. When launching VCV rack it should automatically unpack the plugin

##  Mac x64
1. Download `GL-0.0.2-alpha-mac-x64.vcvplugin`
2. Copy the file into `~/Library/Application Support/Rack2/`
3. When launching VCV rack it should automatically unpack the plugin

## Mac arm64
1. Download `GL-0.0.2-alpha-mac-arm64.vcvplugin`
2. Copy the file into `~/Library/Application Support/Rack2/`
3. When launching VCV rack it should automatically unpack the plugin

## Windows
1. Download `GL-0.0.2-alpha-win-x64.vcvplugin`
2. Copy the file into `C:\Users\<username>\AppData\Local\Rack2\`
3. When launching VCV rack it should automatically unpack the plugin

## Build from source
1. Clone repository
4. Download the Rack SDK for your operating system ([Rack build guide](https://vcvrack.com/manual/Building#Building-Rack-plugins))
5. Unzip Rack SDK and set `RACK_DIR` environment variable to its path: `export RACK_DIR=<RACK_SDK location>`
6. Move into the projects root directory
7. build the plugin with
```
make
```
8. Copy the contents of `/build` into your  [user folder](https://vcvrack.com/manual/FAQ#Where-is-the-Rack-user-folder)
   or alternatively, build the plugin and copy the files to your Rack user folder in one step
```
make install
```