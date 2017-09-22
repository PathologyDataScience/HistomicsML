.. highlight:: shell

============
Installation
============

HistomicsML can be installed from source, but we recommend using the provided Docker image to simplify the process. This image provides a "software container" that is platform independent, and bundled with pre-built libraries and executables.

Installing HistomicsML via Docker
---------------------------------

HistomicsML is implemented as a multi-container image that can be run using docker-compose:

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
   If you already use these ports, you should stop the servers.

The HistomicsML docker can be run on any platform with the following steps:

1. Install docker and docker-compose

* For docker install, refer to https://docs.docker.com/engine/installation/
* For docker-compose install, refer to https://docs.docker.com/compose/install/

2. Clone the HistomicsML source repository and Pull the HistomicsML docker images to your system and start the containers

.. code-block:: bash

  $ git clone https://github.com/CancerDataScience/HistomicsML.git
  $ cd HistomicsML
  $ docker pull cancerdatascience/hmlweb:0.10
  $ docker pull cancerdatascience/hmldb:0.10
  $ docker-compose up -d

3. Confirm that the two containers are running

.. code-block:: bash

  $ docker ps
  CONTAINER ID   IMAGE           COMMAND                  CREATED          STATUS              PORTS                                          NAMES
  97d439b58033   cancerdatascience/hmlweb:0.10   "/bin/sh -c servi..."   2 minutes ago    Up 2 minutes        0.0.0.0:80->80/tcp, 0.0.0.0:20000->20000/tcp   histomicsml_hmlweb_1
  c40e9159dfdb   cancerdatascience/hmldb:0.10    "docker-entrypoint..."   2 minutes ago    Up 2 minutes        0.0.0.0:3306->3306/tcp                         histomicsml_hmldb_1

4. Import sample data into database (the database docker provides sample image data)

.. code-block:: bash

  # Make sure your docker container names are randomly created above.
  # We will use histomicsml_hmldb_1 as a container name.
  $ docker exec -t -i histomicsml_hmldb_1 bash
  root@c40e9159dfdb:/# cd /db
  root@c40e9159dfdb:/db# ./db_run.sh
  ---> Starting MySQL server...
  ---> Sleep start...
  ---> Sleep end
  ---> Data importing start ...
  ---> Data importing end
  root@c40e9159dfdb:/db# exit

5. Check the IP address of the database container

.. code-block:: bash

 $ docker inspect histomicsml_hmldb_1 | grep IPAddress
 SecondaryIPAddresses": null,
          "IPAddress": "",
          "IPAddress": "192.80.0.1",

6. Modify IP address in ``account.php`` in the web container

* Switch to the web docker container and modify the accounts.php file to point to the IP from step 5

.. code-block:: bash

 $ docker exec -t -i histomicsml_hmlweb_1 bash
 root@97d439b58033:/# cd /var/www/html/HistomicsML/db
 root@97d439b58033:/var/www/html/HistomicsML/db# vi account.php
 vi accounts.php

* change "$dbAddress = "192.80.0.2" to "$dbAddress = "192.80.0.1"

7. Start the server

.. code-block:: bash

 root@97d439b58033:/var/www/html/HistomicsML/db# service al_server start
 Starting active learning server daemon al_server [ OK ]
 root@97d439b58033:/var/www/html/HistomicsML/db# exit

.. note:: If the server becomes unresponsive or generates a connection error during use, the al_server will need to be restarted.

8. Navigate your browser to the HistomicsML page http://localhost/HistomicsML.
