#!/bin/sh
#set -e -o pipefail
wdir=`pwd`

OUTDIR=`pwd`
MAKEOPTS=" -j8 "
WRAP_DIR=`pwd`/win_wrapped
HOSTS="x86_64-w64-mingw32"
CONFIGFLAGS="LDFLAGS=-static-libstdc++ --enable-wallet --enable-reduce-exports --disable-bench --disable-gui-tests --disable-tests"
FAKETIME_HOST_PROGS="ar ranlib nm windres strip"
FAKETIME_PROGS="date makensis zip"

export QT_RCC_TEST=0
export QT_RCC_SOURCE_DATE_OVERRIDE=1
export GZIP="-9n"
export TAR_OPTIONS="--mtime="$REFERENCE_DATE\\\ $REFERENCE_TIME""
export TZ="UTC"
export BUILD_DIR=`pwd`
mkdir -p ${WRAP_DIR}
if test -n "$GBUILD_CACHE_ENABLED"; then
  export SOURCES_PATH=${GBUILD_COMMON_CACHE}
  export BASE_CACHE=${GBUILD_PACKAGE_CACHE}
  mkdir -p ${BASE_CACHE} ${SOURCES_PATH}
fi

# Create global faketime wrappers
for prog in ${FAKETIME_PROGS}; do
  echo '#!/bin/bash' > ${WRAP_DIR}/${prog}
  echo "REAL=\`which -a ${prog} | grep -v ${WRAP_DIR}/${prog} | head -1\`" >> ${WRAP_DIR}/${prog}
  echo 'export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/faketime/libfaketime.so.1' >> ${WRAP_DIR}/${prog}
  echo "export FAKETIME=\"${REFERENCE_DATETIME}\"" >> ${WRAP_DIR}/${prog}
  echo "\$REAL \$@" >> $WRAP_DIR/${prog}
  chmod +x ${WRAP_DIR}/${prog}
done

# Create per-host faketime wrappers
for i in $HOSTS; do
  for prog in ${FAKETIME_HOST_PROGS}; do
      echo '#!/bin/bash' > ${WRAP_DIR}/${i}-${prog}
      echo "REAL=\`which -a ${i}-${prog} | grep -v ${WRAP_DIR}/${i}-${prog} | head -1\`" >> ${WRAP_DIR}/${i}-${prog}
      echo 'export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/faketime/libfaketime.so.1' >> ${WRAP_DIR}/${i}-${prog}
      echo "export FAKETIME=\"${REFERENCE_DATETIME}\"" >> ${WRAP_DIR}/${i}-${prog}
      echo "\$REAL \$@" >> $WRAP_DIR/${i}-${prog}
      chmod +x ${WRAP_DIR}/${i}-${prog}
  done
done

# Create per-host linker wrapper
# This is only needed for trusty, as the mingw linker leaks a few bytes of
# heap, causing non-determinism. See discussion in https://github.com/bitcoin/bitcoin/pull/6900
for i in $HOSTS; do
  mkdir -p ${WRAP_DIR}/${i}
  for prog in collect2; do
      echo '#!/bin/bash' > ${WRAP_DIR}/${i}/${prog}
      REAL=$(${i}-gcc -print-prog-name=${prog})
      echo "export MALLOC_PERTURB_=255" >> ${WRAP_DIR}/${i}/${prog}
      echo "${REAL} \$@" >> $WRAP_DIR/${i}/${prog}
      chmod +x ${WRAP_DIR}/${i}/${prog}
  done
  for prog in gcc g++; do
      echo '#!/bin/bash' > ${WRAP_DIR}/${i}-${prog}
      echo "REAL=\`which -a ${i}-${prog}-posix | grep -v ${WRAP_DIR}/${i}-${prog} | head -1\`" >> ${WRAP_DIR}/${i}-${prog}
      echo 'export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/faketime/libfaketime.so.1' >> ${WRAP_DIR}/${i}-${prog}
      echo "export FAKETIME=\"${REFERENCE_DATETIME}\"" >> ${WRAP_DIR}/${i}-${prog}
      echo "export COMPILER_PATH=${WRAP_DIR}/${i}" >> ${WRAP_DIR}/${i}-${prog}
      echo "\$REAL \$@" >> $WRAP_DIR/${i}-${prog}
      chmod +x ${WRAP_DIR}/${i}-${prog}
  done
done

export PATH=${WRAP_DIR}:${PATH}

if [ ! -d $HOSTS ]; then
  tar -xf $HOSTS.tbz
  cd $HOSTS
  find . -name '*.la' -exec sed -i "s#/root/dcwin/win_depends#${BUILD_DIR}#g" {} \;
  find . -name '*.pc' -exec sed -i "s#/root/dcwin/win_depends#${BUILD_DIR}#g" {} \;
  find . -name '*.h' -exec sed -i "s#/root/dcwin/win_depends#${BUILD_DIR}#g" {} \;
  cd ..
fi

cd $wdir/../
BASEPREFIX=$wdir

# Build dependencies for each host
# for i in $HOSTS; do
#   make ${MAKEOPTS} -C ${BASEPREFIX} HOST="${i}"
# done

# Create the release tarball using (arbitrarily) the first host
./autogen.sh
./configure $CONFIGFLAGS --prefix=${BASEPREFIX}/`echo "${HOSTS}" | awk '{print $1;}'`
make dist
# cp bitcoinUnlimited-*.tar.gz Diskcoin-1.1.0.tar.gz
SOURCEDIST=`echo Diskcoin-*.tar.gz`
DISTNAME=`echo ${SOURCEDIST} | sed 's/.tar.*//'`

mv $SOURCEDIST $wdir/
# Correct tar file order
# mkdir -p temp
# cd temp
# tar xf ../$SOURCEDIST
# find Diskcoin* | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ../$SOURCEDIST
# # mkdir -p $OUTDIR/src
# # cp ../$SOURCEDIST $OUTDIR/src
# cd ..
# mv $SOURCEDIST $wdir/
# rm -fr temp

ORIGPATH="$PATH"
# Extract the release tarball into a dir for each host and build
for i in ${HOSTS}; do
  export PATH=${BASEPREFIX}/${i}/native/bin:${ORIGPATH}
  mkdir -p $BASEPREFIX/distsrc-${i}
  rm -fr $BASEPREFIX/distsrc-${i}/*
  cd $BASEPREFIX/distsrc-${i}
  INSTALLPATH=`pwd`/installed/${DISTNAME}
  mkdir -p ${INSTALLPATH}
  tar --strip-components=1 -xf $wdir/$SOURCEDIST

  ./configure --prefix=${BASEPREFIX}/${i} --bindir=${INSTALLPATH}/bin --includedir=${INSTALLPATH}/include --libdir=${INSTALLPATH}/lib --disable-ccache --disable-maintainer-mode --disable-dependency-tracking ${CONFIGFLAGS}
  make ${MAKEOPTS}
  make ${MAKEOPTS} -C src # check-security
  make deploy
  make install-strip
  cp -f Diskcoin*setup*.exe $OUTDIR/diskcoin-w64.exe
  # cd installed
  # mv ${DISTNAME}/bin/*.dll ${DISTNAME}/lib/
  # find . -name "lib*.la" -delete
  # find . -name "lib*.a" -delete
  # rm -rf ${DISTNAME}/lib/pkgconfig
  # find ${DISTNAME} -type f | sort | zip -X@ ${OUTDIR}/${DISTNAME}-${i}.zip
  # cd ../..
done
rm -f $wdir/$SOURCEDIST

# rename 's/-setup\.exe$/-setup-unsigned.exe/' *-setup.exe
# find . -name "*-setup-unsigned.exe" | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ${OUTDIR}/${DISTNAME}-win-unsigned.tar.gz
# mv ${OUTDIR}/${DISTNAME}-x86_64-*.zip ${OUTDIR}/${DISTNAME}-win64.zip
# mv ${OUTDIR}/${DISTNAME}-i686-*.zip ${OUTDIR}/${DISTNAME}-win32.zip

