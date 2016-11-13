cd $SOFTWARE/installation
tar -xvzf szip-2.1.tar.gz
tar -xvzf hdf5-1.10.0-patch1.tar.gz

mkdir $SOFTWARE/HDF5
mkdir $SOFTWARE/SZIP

cd $SOFTWARE/installation/szip-2.1
./configure --prefix=$SOFTWARE/SZIP
make & make install

cd $SOFTWARE/installation/hdf5-1.10.0-patch1
./configure --prefix=$SOFTWARE/HDF5 --with-szlib=$SOFTWARE/SZIP
make & make install
mv $SOFTWARE/installation/hdf5-1.10.0-patch1/c++/src/h5c++ $SOFTWARE/HDF5/bin/

dtc -O dtb -I dts -o /lib/firmware/PRU-SPI-00A0.dtbo -b 0 -@ $SOFTWARE/installation/PRU-SPI-00A0.dts
dtc -O dtb -I dts -o /lib/firmware/PRU-ENABLE-00A0.dtbo -b 0 -@ $SOFTWARE/installation/PRU-ENABLE-00A0.dts

#pasm -b ADS8329.p
#$SOFTWARE/HDF5/bin/h5cc -o ADS8329 ADS8329.c -lprussdrv -lpthread
