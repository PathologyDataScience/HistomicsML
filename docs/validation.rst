.. highlight:: shell

============
Validating a classifier
============

This page documents how to create validation datasets and how to use validation datasets to measure classifier accuracy.

Generating a validation set
------------------------------

Direct your browser to http://localhost/HistomicsML/validation.html?application=nuclei/.

Under *Start a validation set* enter a test set name and choose the dataset you want to select validation examples from. Enter the names
of your positive and negative classes.

.. image:: images/test-1.png

.. note:: A strict validation procedure should perform training and validation on separate images. A slide collection can be artificially split
prior to import to produce separate training and validation datasets.

The validation interface is similar to the ``Prime`` interface, providing a slide selector, a slide viewer, and a thumbnail gallery of annotated objects.
Object boundaryies can be displayed by selecting ``Show Segmentation``. Clicking ``Select Nuclei`` will activate the cursor to begin annotating objects.
Choose the object class that you want to annotate using the radio button, and then select objects by double-clicking within their boundaries in the slide viewer.

.. image:: images/test-2.png

.. note:: Thumbnail images of validation set objects are displayed at the top of the screen. 
    The class label of these objects can be toggled with a single-click. 
    A double-click will remove the object from the validation set.

Clicking ``Add`` will commit these objects to the validation set. The ``Save`` button will save the validation set to the database.


Validating a classifier
------------------------------

The ``Validation`` interface also enables users to measure classifier accuracy by applying a trained classifier to a validation dataset.

At the ``Validation`` menu under *Validate classifier*, select the training dataset and classifier name, and the validation dataset and validation set name.

The ``Validate`` button will generate a .csv file containing: 1. The predicted class labels for each object in the validation set (1 - positive, -1 - negative) 2. The prediction scores from the random forest classifier (ranged 0-1) 3. The false-positive rate 4. The true-positive rate 5. The precision and 6. the total accuracy.
