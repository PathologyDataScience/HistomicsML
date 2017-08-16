.. highlight:: shell

============
Installation
============

To install HistomicsML on your system, we recommend you to install our docker
container. Because the library dependencies of OpenJPEG, libtiff,
and other numerous packages, it may be hard to directly install HistomicsML on your system.
Here, we provide a docker container to avoid software conflicts and
library compatibility issues.

Installing HistomicsTK via Docker
---------------------------------

Tree structure for HistomicsML docker is as below.

.. code-block:: bash

  /HistomicsML
  │
  ├── hmlweb:latest
  │
  ├── hmldb:latest
  │
  └── docker-compose.yml

* /HistomicsML: a working directory on your system.
* hmlweb:latest: a docker image for HistomicsML web server.
* hmldb:latest: a docker image for HistomcisML database.
* docker-compose.yml: a file for defining and running docker containers.

Now, we describe how to install HistomicsML using docker container.

1. Install docker and docker-compose:

* For docker install, refer to https://docs.docker.com/engine/installation/
* For docker-compose install, refer to https://docs.docker.com/compose/install/

2. Clone source codes and Pull HistomicsML docker images:

.. code-block:: bash

  $ git clone https://github.com/CancerDataScience/HistomicsML.git
  $ cd HistomicsML
  $ docker pull histomicsml/hmlweb:latest
  $ docker pull histomicsml/hmldb:latest

3. Run docker images:

.. code-block:: bash

  $ docker-compose up -d

4. Check if two containers are correctly running:

.. code-block:: bash

  $ docker ps
  CONTAINER ID   IMAGE           COMMAND                  CREATED          STATUS              PORTS                                          NAMES
  7864853c4e38   hmlweb:latest   "/bin/sh -c servi..."   2 minutes ago    Up 2 minutes        0.0.0.0:80->80/tcp, 0.0.0.0:20000->20000/tcp   dockernuclei_web_1
  19cd8ef3e1ec   hmldb:latest    "docker-entrypoint..."   2 minutes ago    Up 2 minutes        0.0.0.0:3306->3306/tcp                         dockernuclei_db_1

5. Import data into database:

.. code-block:: bash

  $ docker exec -t -i dockernuclei_db_1 bash
  root@19cd8ef3e1ec:/# cd /db
  root@19cd8ef3e1ec:/# ./db_run.sh

6. Check IP address of ``dockernuclei_db_1`` container:

.. code-block:: bash

 $ docker inspect dockernuclei_db_1 | grep IPAddress
 SecondaryIPAddresses": null,
          "IPAddress": "",
          "IPAddress": "192.80.0.1",

7. Modify IP address of dbAddress in ``dockernuclei_hmlweb_1`` container:

.. code-block:: bash

 $ docker exec -t -i dockernuclei_hmlweb_1 bash
 root@19cd8ef3e1ec:/# cd /var/www/html/HistomicsML/db

 * Open up the account.php in your text editor and modify $dbAddress.
 * $dbAddress = "192.80.0.2"; => $dbAddress = "192.80.0.1"

8. Start learning server:

.. code-block:: bash

 root@19cd8ef3e1ec:/# service al_server start

9. Run HistomicsML: http::/localhost/HistomicsML
