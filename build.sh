#! /bin/bash

TIME_BEGIN=$( date -u +%s )
RED='\033[0;31m'
NC='\033[0m'

CWD=`pwd`

if [ ! -d "/usr/local/eosio.wasmsdk" ]; then
    cd eosio.wasmsdk
	git submodule update --init --recursive
    sed "s/bash .\/scripts/echo 1 | bash .\/scripts/g;s/\${CORE_SYMBOL}/DMC/g" build.sh | bash
    cd build
    sudo make install
    cd ${CWD}
fi

printf "\t=========== Building dmc.contracts ===========\n\n"
unamestr=`uname`
if [[ "${unamestr}" == 'Darwin' ]]; then
   BOOST=/usr/local
   CXX_COMPILER=g++
else
   BOOST=~/opt/boost
	OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )

	case "$OS_NAME" in
		"Amazon Linux AMI")
			CXX_COMPILER=g++
			C_COMPILER=gcc
		;;
		"CentOS Linux")
			CXX_COMPILER=g++
			C_COMPILER=gcc
		;;
		"elementary OS")
			CXX_COMPILER=clang++-4.0
			C_COMPILER=clang-4.0
		;;
		"Fedora")
			CXX_COMPILER=g++
			C_COMPILER=gcc
		;;
		"Linux Mint")
			CXX_COMPILER=clang++-4.0
			C_COMPILER=clang-4.0
		;;
		"Ubuntu")
			CXX_COMPILER=clang++-4.0
			C_COMPILER=clang-4.0
		;;
		*)
			printf "\\n\\tUnsupported Linux Distribution. Exiting now.\\n\\n"
			exit 1
	esac
fi

CORES=`getconf _NPROCESSORS_ONLN`
mkdir -p build
pushd build &> /dev/null
cmake -DCXX_COMPILER="${CXX_COMPILER}" -DBOOST_ROOT="${BOOST}" -DEOSIO_INSTALL_PREFIX=/usr/local ../dmc.contracts
make -j${CORES}
popd &> /dev/null
TIME_END=$(( $(date -u +%s) - ${TIME_BEGIN} ))
printf "\\n\\tdmc.contracts has been successfully built. %02d:%02d:%02d\\n\\n" $(($TIME_END/3600)) $(($TIME_END%3600/60)) $(($TIME_END%60))
