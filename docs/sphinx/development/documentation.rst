=============
Documentation
=============

.. contents:: Table of Contents
   :local:
   :depth: 2

Documentation System
====================

The project uses Sphinx for user documentation and Doxygen for API documentation.

Building Documentation
----------------------

.. code-block:: bash

   # Install dependencies
   pip install -r docs/requirements.txt
   
   # Build all documentation
   cmake --build build --target docs

Documentation Structure
=======================

- **User Guide**: Getting started and usage
- **Architecture**: System design and patterns
- **API Reference**: Generated from code
- **Development**: Contributing guidelines

.. seealso::
   - This documentation system itself!