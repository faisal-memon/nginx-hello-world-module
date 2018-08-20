FROM nginx:latest

RUN apt-get update && apt-get install -y apt-utils \
                                         autoconf \
                                         automake \
                                         build-essential \
                                         git \
                                         libcurl4-openssl-dev \
                                         libgeoip-dev \
                                         liblmdb-dev \
                                         libpcre++-dev \
                                         libtool \
                                         libxml2-dev \
                                         libyajl-dev \
                                         pkgconf \
                                         wget \
                                         zlib1g-dev

ADD src nginx-hello-world-module

RUN export NGINX_VERSION_SHORT=`echo ${NGINX_VERSION} | sed 's/-.*//'` && \
    wget http://nginx.org/download/nginx-${NGINX_VERSION_SHORT}.tar.gz && \
    tar zxvf nginx-${NGINX_VERSION_SHORT}.tar.gz && \
    rm -f /nginx-${NGINX_VERSION_SHORT}.tar.gz

RUN export NGINX_VERSION_SHORT=`echo ${NGINX_VERSION} | sed 's/-.*//'` &&\
    cd nginx-${NGINX_VERSION_SHORT} && \
    ./configure --with-compat --add-dynamic-module=../nginx-hello-world-module && \
    make modules && \
    cp objs/ngx_http_hello_world_module.so /etc/nginx/modules

# Modify /etc/nginx/nginx.conf to dynamically load the module.
RUN sed -i "5i load_module \"modules/ngx_http_hello_world_module.so\";\n" /etc/nginx/nginx.conf

ADD hello_world.conf /etc/nginx/conf.d/hello_world.conf
ADD langs.dat /etc/nginx/conf.d/langs.dat
