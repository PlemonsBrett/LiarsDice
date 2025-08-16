DOXYFILE = 'Doxyfile'

# Main navigation structure
LINKS_NAVBAR1 = [
    ("Modules", 'modules', []),
    ("Namespaces", 'namespaces', []),
    ("Classes", 'annotated', []),
]

# Class documentation
LINKS_NAVBAR2 = [
    ("Files", 'files', []),
]

# m.css theme configuration
M_THEME_COLOR = '#22272e'

# Project information (renamed to avoid conflicts with m.css)
DOXYGEN_PROJECT_NAME = "Liar's Dice"
DOXYGEN_PROJECT_BRIEF = "A modern C++20 implementation of the classic dice game with AI players"

# HTML output configuration
HTML_HEADER = '''
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
'''

# Enable search
M_SEARCH_DISABLED = False

# Show class hierarchy
M_CLASS_TREE_EXPAND_LEVELS = 2

# Code highlighting
M_CODE_HIGHLIGHTING_ENABLED = True

# Math rendering (if needed)
M_MATH_RENDER_AS_CODE = False
