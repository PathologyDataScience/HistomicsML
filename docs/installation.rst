.. highlight:: shell

============
Installation
============

To install HistomicsML on your system, we recommend you to install our docker
container. Because the library dependencies of OpenJPEG, libtiff,
and other numerous packages, it may be hard to directly install HistomicsML on your system.
Here, we provide a docker container to avoid software conflicts and
library compatibility issues.

Installing HistomicsML via Docker
---------------------------------

Tree structure for HistomicsML docker is as below.

.. code-block:: bash

  /HistomicsML
  │
  ├── hmlweb:0.10
  │
  ├── hmldb:0.10
  │
  └── docker-compose.yml

* /HistomicsML: a working directory on your system.
* hmlweb:0.10: a docker image for HistomicsML web server.
* hmldb:0.10: a docker image for HistomcisML database.
* docker-compose.yml: a file for defining and running docker containers.


.. note:: Apache and Mysql servers on HistomicsML docker run on Port 80 and 3306 respectively.
   If you already use the ports, you should stop the servers.

Now, we describe how to install HistomicsML using docker container.

1. Install docker and docker-compose:

* For docker install, refer to https://docs.docker.com/engine/installation/
* For docker-compose install, refer to https://docs.docker.com/compose/install/

2. Clone source codes and Pull HistomicsML docker images:

.. code-block:: bash

  $ git clone https://github.com/CancerDataScience/HistomicsML.git
  $ cd HistomicsML
  $ docker pull cancerdatascience/hmlweb:0.10
  $ docker pull cancerdatascience/hmldb:0.10

3. Run docker images:

.. code-block:: bash

  $ docker-compose up -d

4. Check if two containers are correctly running:

.. code-block:: bash

  $ docker ps
  CONTAINER ID   IMAGE           COMMAND                  CREATED          STATUS              PORTS                                          NAMES
  97d439b58033   cancerdatascience/hmlweb:0.10   "/bin/sh -c servi..."   2 minutes ago    Up 2 minutes        0.0.0.0:80->80/tcp, 0.0.0.0:20000->20000/tcp   docker_hmlweb_1
  c40e9159dfdb   cancerdatascience/hmldb:0.10    "docker-entrypoint..."   2 minutes ago    Up 2 minutes        0.0.0.0:3306->3306/tcp                         docker_hmldb_1

5. Import data into database:

.. code-block:: bash

  # Make sure your docker container names are randomly created above.
  # We will use docker_hmldb_1 as a container name.
  $ docker exec -t -i docker_hmldb_1 bash
  root@c40e9159dfdb:/# cd /db
  root@c40e9159dfdb:/db# ./db_run.sh
  ---> Starting MySQL server...
  ---> Sleep start...
  ---> Sleep end
  ---> Data importing start ...
  ---> Data importing end
  root@c40e9159dfdb:/db# exit

6. Check IP address of ``docker_hmldb_1`` container:

.. code-block:: bash

 $ docker inspect docker_hmldb_1 | grep IPAddress
 SecondaryIPAddresses": null,
          "IPAddress": "",
          "IPAddress": "192.80.0.1",

7. Modify IP address in ``account.php`` on ``docker_hmlweb_1`` container:

.. code-block:: bash

 $ docker exec -t -i docker_hmlweb_1 bash
 root@97d439b58033:/# cd /var/www/html/HistomicsML/db
 root@97d439b58033:/# cd /var/www/html/HistomicsML/db
 root@97d439b58033:/var/www/html/HistomicsML/db# vi account.php

 * Open up the account.php in your text editor and modify $dbAddress.
 * $dbAddress = "192.80.0.2"; => $dbAddress = "192.80.0.1"

8. Start learning server:

.. code-block:: bash

 root@97d439b58033:/var/www/html/HistomicsML/db# service al_server start
 Starting active learning server daemon al_server [ OK ]
 root@97d439b58033:/var/www/html/HistomicsML/db# exit

9. Run HistomicsML http://localhost/HistomicsML.
