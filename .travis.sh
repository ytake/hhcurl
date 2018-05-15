#!/bin/bash
set -ex
hhvm --version
curl https://getcomposer.org/installer | hhvm -d hhvm.jit=0 --php -- /dev/stdin --install-dir=/usr/local/bin --filename=composer

cd tests/server/; hhvm -d hhvm.server.type=proxygen -d hhvm.server.port=8080 -d hhvm.server.source_root=./ -m server &

cd /var/source
hhvm -d hhvm.php7.all=1 -d hhvm.jit=0 -d hhvm.hack.lang.auto_typecheck=0 /usr/local/bin/composer install
hh_client
hhvm -d hhvm.php7.all=1 -d hhvm.jit=0 -d hhvm.hack.lang.auto_typecheck=0 vendor/bin/phpunit
