pushd "%~dp0"
set cwd="%cd%\.."
popd

pushd %cwd%

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -Bbuild . -G "Ninja"

popd