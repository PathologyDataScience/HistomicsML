.. highlight:: shell

==============
Importing datasets
==============

A data import interface enables users to easily import dataset into the system. This page demonstrates the data import function using the sample data located on the database container.

The whole-slide image and the boundary, feature, and slide information files are located in separate folders on the database container

.. code-block:: bash

  /fastdata/features/GBM/
  │
  ├── GBM-boundaries.txt
  │
  ├── GBM-features.h5
  │
  └── GBM-pyramids.csv

  /localdata/pyramids/GBM/
  │
  └── TCGA-02-0010-01Z-00-DX4.svs.dzi.tif

The following steps, the interface is used to import this dataset into this system

1. Create a folder on the container and modify permissions to enable import

.. code-block:: bash

 $ docker exec -t -i histomicsml_hmlweb_1 bash
 root@19cd8ef3e1ec:/# cd /fastdata/features
 root@19cd8ef3e1ec:/fastdata/features# mkdir NewProjectDirectory
 root@19cd8ef3e1ec:/fastdata/features# chmod 777 NewProjectDirectory

3. Copy the sample data to ``NewProjectDirectory``

.. code-block:: bash

  root@19cd8ef3e1ec:/fastdata/features# cd NewProjectDirectory
  root@19cd8ef3e1ec:/fastdata/features/NewProjectDirectory# cp ../GBM/* ./
  # This copies GBM-boundaries.txt, GBM-features.h5, GBM-pyramids.csv to NewProjectDirectory
  root@97d439b58033:/fastdata/features/NewProjectDirectory# mkdir BoundaryDirectory
  root@97d439b58033:/fastdata/features/NewProjectDirectory# mv GBM-boundaries.txt BoundaryDirectory/
  # This moves the sample boundary file to BoundaryDirectory under NewProjectDirectory

4. Import dataset using the web interface

* Open the web page http://localhost/HistomicsML/data.html?application=nuclei
* Enter a dataset name and select ``NewProjectDirectory`` from the Project Directory dropdown.
* The remaining fields will automatically populate once the directory is selected.

.. image:: images/import.png

* Click Submit to confirm the import

Now, you can see the new dataset on the main page, http://localhost/HistomicsML.

.. image:: images/main.png


The import interface can also be used to delete an existing dataset from the system

* To delete the current dataset, go to http://localhost/HistomicsML/data.html?application=nuclei and select the current dataset from the dropdown on the top right, and then click Remove button.

.. image:: images/delete.png
