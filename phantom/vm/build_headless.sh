export PHANTOM_HOME=/home/anton/Sandbox/PhantomOS/phantomuserland

pushd $PHANTOM_HOME
pushd phantom

pushd gl
make clean
popd

pushd libtuned
make clean
make
popd

pushd libphantom
make clean
make
popd

pushd libwin
make clean
make
popd

pushd libfreetype
make clean
make
popd

pushd libc
make clean
make
popd

pushd vm
make clean
make pvm_headless
popd

popd
popd
