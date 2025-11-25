git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg.exe install cpr nlohmann-json
.\vcpkg.exe integrate install
.\vcpkg.exe list 