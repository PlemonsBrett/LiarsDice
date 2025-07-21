# Configuration file for the Sphinx documentation builder.
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import sys
from datetime import datetime

project = 'LiarsDice Game Engine'
copyright = f'{datetime.now().year}, Brett Plemons'
author = 'Brett Plemons'
release = '1.0.0'
version = '1.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.napoleon',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'sphinx.ext.intersphinx',
    'sphinx.ext.autosummary',
    'sphinx.ext.doctest',
    'sphinx.ext.mathjax',
    'sphinx.ext.ifconfig',
    'sphinx.ext.githubpages',
    'breathe',  # For Doxygen integration
    'myst_parser',  # For Markdown support
    'sphinxcontrib.mermaid',  # For Mermaid diagrams
    'sphinx_tabs.tabs',  # For tabbed content
]

# Breathe configuration for Doxygen integration
breathe_projects = {
    "LiarsDice": "../api/xml"
}
breathe_default_project = "LiarsDice"

# MyST parser configuration
myst_enable_extensions = [
    "deflist",
    "tasklist", 
    "html_admonition",
    "html_image",
    "colon_fence",
    "smartquotes",
    "replacements",
    "linkify",
    "strikethrough",
    "substitution",
]

# Templates path
templates_path = ['_templates']

# Exclude patterns
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# Source file suffixes
source_suffix = {
    '.rst': None,
    '.md': 'myst_parser',
}

# Master document
master_doc = 'index'

# Language
language = 'en'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

# Theme options
html_theme_options = {
    'canonical_url': '',
    'analytics_id': '',
    'logo_only': False,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'vcs_pageview_mode': '',
    'style_nav_header_background': '#2980b9',
    # TOC options
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

# Custom CSS
html_css_files = [
    'custom.css',
]

# HTML title
html_title = f"{project} v{version}"

# HTML short title
html_short_title = project

# HTML logo
# html_logo = '_static/logo.png'

# HTML favicon
# html_favicon = '_static/favicon.ico'

# Additional HTML context
html_context = {
    "display_github": True,
    "github_user": "bplemons",
    "github_repo": "LiarsDice",
    "github_version": "main",
    "conf_py_path": "/docs/sphinx/",
}

# -- Options for LaTeX output ------------------------------------------------
latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    'papersize': 'letterpaper',
    
    # The font size ('10pt', '11pt' or '12pt').
    'pointsize': '10pt',
    
    # Additional stuff for the LaTeX preamble.
    'preamble': '',
    
    # Latex figure (float) alignment
    'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files
latex_documents = [
    (master_doc, 'LiarsDice.tex', 'LiarsDice Game Engine Documentation',
     'Brett Plemons', 'manual'),
]

# -- Options for manual page output ------------------------------------------
man_pages = [
    (master_doc, 'liarsdice', 'LiarsDice Game Engine Documentation',
     [author], 1)
]

# -- Options for Texinfo output ----------------------------------------------
texinfo_documents = [
    (master_doc, 'LiarsDice', 'LiarsDice Game Engine Documentation',
     author, 'LiarsDice', 'Modern C++23 implementation of Liar\'s Dice.',
     'Miscellaneous'),
]

# -- Options for Epub output -------------------------------------------------
epub_title = project
epub_author = author
epub_publisher = author
epub_copyright = copyright

# -- Extension configuration -------------------------------------------------

# Napoleon settings
napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = False
napoleon_include_private_with_doc = False
napoleon_include_special_with_doc = True
napoleon_use_admonition_for_examples = False
napoleon_use_admonition_for_notes = False
napoleon_use_admonition_for_references = False
napoleon_use_ivar = False
napoleon_use_param = True
napoleon_use_rtype = True
napoleon_preprocess_types = False
napoleon_type_aliases = None
napoleon_attr_annotations = True

# Todo extension settings
todo_include_todos = True

# Intersphinx mapping
intersphinx_mapping = {
    'python': ('https://docs.python.org/3/', None),
    # Note: cppreference.com doesn't provide objects.inv, commenting out
    # 'cpp': ('https://en.cppreference.com/w/', None),
}

# Autodoc settings
autodoc_member_order = 'bysource'
autodoc_default_flags = ['members', 'undoc-members', 'show-inheritance']
autodoc_mock_imports = []

# Coverage settings
coverage_show_missing_items = True

# Autosummary settings
autosummary_generate = True
autosummary_imported_members = True

# Doctest settings
doctest_global_setup = '''
import sys
import os
'''

# HTML search options
html_search_language = 'en'
html_search_options = {'type': 'default'}

# Version info
# The full version, including alpha/beta/rc tags
release = version

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# If false, no module index is generated.
html_domain_indices = True

# If false, no index is generated.
html_use_index = True

# Split the index
html_split_index = False

# If true, links to the reST sources are added to the pages.
html_show_sourcelink = True

# If true, "Created using Sphinx" is shown in the HTML footer.
html_show_sphinx = True

# If true, "(C) Copyright ..." is shown in the HTML footer.
html_show_copyright = True

# If true, an OpenSearch description file will be output
html_use_opensearch = ''

# Output file base name for HTML help builder.
htmlhelp_basename = 'LiarsDicedoc'