.. HistomicsML documentation master file, created by
   sphinx-quickstart on Tue Aug 15 10:00:40 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

HistomicsML: An interactive machine-learning framework
========================================================

A step-by-step guide to exploring HistomicsML.

Installation
~~~~~~~~~~~~~

1. Install docker and docker-compose:

 * For docker install, refer to https://docs.docker.com/engine/installation/
 * For docker-compose install, refer to https://docs.docker.com/compose/install/

2. Pull HistomicsML docker images (hmlweb, hmldb):
::

$ mkdir /home/HistomicsML
$ docker pull histomicsml/hmlweb:latest
$ docker pull histomicsml/hmldb:latest

3. Run docker:
::

$ docker-compose up -d

4. Check if two containers are correctly running:

.. code-block:: bash

 $ docker ps
 CONTAINER ID   IMAGE           COMMAND                  CREATED          STATUS              PORTS                                          NAMES
 7864853c4e38   hmlweb:latest   "/bin/sh -c servi..."   2 minutes ago    Up 2 minutes        0.0.0.0:80->80/tcp, 0.0.0.0:20000->20000/tcp   docker_web_1
 19cd8ef3e1ec   hmldb:latest    "docker-entrypoint..."   2 minutes ago    Up 2 minutes        0.0.0.0:3306->3306/tcp                         docker_db_1

5. Insert data into database:

.. code-block:: bash

 $ docker exec -t -i docker_hmldb_1 bash
 root@19cd8ef3e1ec:/# cd /db
 root@19cd8ef3e1ec:/# ./db_run.sh

6. Find IP address of docker_hmldb_1 container:

.. code-block:: bash

 $ docker inspect docker_hmldb_1 | grep IPAddress
 SecondaryIPAddresses": null,
          "IPAddress": "",
          "IPAddress": "192.80.0.1",

7. Modify docker_hmlweb_1 container:

.. code-block:: bash

 $ docker exec -t -i docker_hmlweb_1 bash
 root@19cd8ef3e1ec:/# cd /var/www/html/HistomicsML/db

 * Open up the account.php in your text editor and modify $dbAddress.
 * $dbAddress = "192.80.0.2"; => $dbAddress = "192.80.0.1"

8. Start learning server:

.. code-block:: bash

 root@19cd8ef3e1ec:/# service al_server start

9. Run HistomicsML: http::/localhost/HistomicsML


Developer installation
~~~~~~~~~~~~~~~~~~~~~~~





.. toctree::
   :maxdepth: 2
   :caption: Contents:



Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
