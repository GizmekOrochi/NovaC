Low-level memory model
======================

This example connects NovaC's exact-bit type/layout/storage facilities to the
Memory asset. ``CardinalDirection`` occupies exactly two bits with one-bit
alignment, so four values pack at offsets 0, 2, 4, and 6.

The address space is backed by ``BitStorageAccess`` here, but ``BitAccess`` is a
polymorphic boundary: a language can replace the backend with device memory,
mirrored storage, a simulator, or another custom bit-addressable implementation.

.. literalinclude:: ../../examples/memory_model.cpp
   :language: cpp
   :linenos:

Expected output
---------------

.. code-block:: text

   offsets=0,2,4,6
   packed=228
   managed-after-end=dead

The typed ``load``/``store`` path validates provenance and lifetime metadata,
then delegates value encoding to ``StorageController``. Raw ``loadBits`` and
``storeBits`` stay available independently for deliberately unmanaged low-level
access.
