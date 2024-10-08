pushd "%~dp0"
set cwd="%cd%\.."
popd

pushd %cwd%

cmake --build build

popd
