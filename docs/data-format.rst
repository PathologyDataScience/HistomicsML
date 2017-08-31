.. highlight:: shell

==============
Data format
==============

A dataset of HistomicsML consists of the whole-slide images, slide information,
object boundaries and extracted features.
For example, our sample dataset consists of the whole-slide image (.tif),
slide information (.csv), cell boundaries (.txt), and the features (.h5).

.. note:: Scripts specific to the dataset need to be created
   since the extracted features and boundaries may not be provided in the same format.



Whole-slide images
------------------

Whole-slide images can be handled on the IIPImage server (http://iipimage.sourceforge.net/documentation/server/).
IIPImage server requires the whole-slide images to be formatted in a multi-resolution 'pyramid'.
We have used Vips (http://www.vips.ecs.soton.ac.uk/index.php?title=VIPS)
to do the conversion.

.. note:: The path to the image needs to be saved in the database.
   HistomicsML uses the database to get the path when forming a request for the IIPIMage server.



Slide information
------------------------------------
A table (.csv) will be needed for each slide information.
The table format is as below:

.. code-block:: bash

  <slide name>,<width in pixels>,<height in pixels>,<path to the pyramid on IIPServer>,<scale>

where scale = 1 for 20x and 2 for 40x.

For example, our sample file (GBM-pyramids.csv) is shown as below:

.. code-block:: bash

   TCGA-02-0010-01Z-00-DX4,32001,38474,/fastdata/pyramids/GBM/TCGA-02-0010-01Z-00-DX4.svs.dzi.tif,1



Boundaries
----------
The boundary format is as below:

.. code-block:: bash

  <slide name> \t <centroid x coordinate> \t <centroid y coordinate> \t <boundary points>

where \t is a tab character and <boundary points> are formatted as:
x1,y1 x2,y2 x3,y3 ... xN,yN (Spaces between coordinates, no spaces within coordinates)

For example, our sample file (GBM-boundaries.txt) is shown as below:

.. code-block:: bash

  TCGA-02-0010-01Z-00-DX4 2250.1 4043.0 2246,4043 2247,4043 2247 ... 2247,4043 2246,4043



Features
--------

We have used HDF5 format to store the features. The format is as below:

.. code-block:: bash

  /dataIdx - Index into the features data of the first sample from the corresponding slide.
  /features - Vector of floats representing the features. Each row is a sample, samples from the same slide/image are contiguous. Data is normalized using z-score.
  /mean -  Mean value of each feature. (Used for z-score normalization)
  /std_dev - Standard deviation of each feature. (Used for z-score normalization)
  /slides	-	Names of the slides/images in the dataset
  /slideIdx - Index into the slides data for each sample, 0 is the first slide, 1 the sceond...
  /x_centroid - Float, x location of the sample in the image.
  /y_centroid - Float, y location of the sample in the image.

You can see our sample file (GBM-features.h5) provided in the docker container,
following the command.

.. code-block:: python

  >>> import h5py
  >>> file="GBM-features.h5"
  >>> contents = h5py.File(file)
  >>> for i in contents:
  ...     print i
  ...
  # for loop will print out the feature information under the root of HDF5.

  dataIdx
  features
  mean
  slideIdx
  slides
  std_dev
  x_centroid
  y_centroid

  #for further step, if you want to see the details.

  >>> contents['features'][0]
  array([ -7.30991781e-01,  -8.36540878e-01,  -1.07858682e+00,
         9.26770031e-01,  -9.31272805e-01,  -4.36136842e-01,
        -1.13033086e-01,   5.28297901e-01,   6.85962856e-01,
         5.07918596e-01,  -5.27561486e-01,  -7.48096228e-01,
        -6.84849143e-01,  -8.79032671e-01,  -1.41368553e-01,
        -3.24195564e-01,  -4.50991303e-01,  -1.32366025e+00,
         9.17324543e-01,   8.36400129e-03,  -2.92657673e-01,
         2.01028720e-01,  -1.93680093e-01,   8.68237793e-01,
         5.72155595e-01,   3.29810083e-01,  -3.63551527e-01,
        -2.87026823e-01,  -8.47819634e-03,  -4.55458522e-01,
         1.43787396e+00,   5.24487114e+00,  -9.62561846e-01,
         5.94001710e-01,   3.57634330e+00,  -2.94562435e+00,
        -9.18125820e+00,   2.87391472e+01,  -9.34123135e+00,
         2.55983505e+01,  -2.99653459e+00,  -1.17376029e-01,
        -5.40324259e+00,   1.01094952e+01,   5.87054205e+00,
         6.21094942e+00,  -2.59355903e+00,  -4.27142763e+00], dtype=float32)
