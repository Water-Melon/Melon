FROM ubuntu
WORKDIR /root
RUN apt-get update && \
    apt-get -y install make git gcc libmysqlclient-dev && \
    git clone https://github.com/Water-Melon/Melang.git && \
    git clone https://github.com/Water-Melon/Melon.git && cd Melon && ./configure && make && make install && \
    echo "/usr/local/melon/lib/" >>  /etc/ld.so.conf && ldconfig && \
    ln -s /usr/local/melon/conf /etc/melon && \
    ln -s /usr/local/melon/lib/libmelon* /usr/lib/ && \
    ln -s /usr/local/melon/include /usr/include/melon && \
    cd ../Melang && ./configure && make all && make install && \
    cd ../ && rm -fr Melon Melang
