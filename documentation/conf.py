DOXYFILE = 'Doxyfile'

# Main navigation structure
LINKS_NAVBAR1 = [
    ("Home", 'index', []),
    ("Modules", 'modules', []),
    ("Pages", 'pages', [
        ('About', 'about'),
        ('Architecture', 'architecture'),
        ('AI System', 'ai_system'),
        ('Game Rules', 'game_rules'),
        ('Technical Notes', 'technical_notes'),
    ]),
    ("Namespaces", 'namespaces', [
        (None, 'namespaceliarsdice'),
        (None, 'namespaceliarsdice_1_1core'),
        (None, 'namespaceliarsdice_1_1ai'),
        (None, 'namespaceliarsdice_1_1config'),
        (None, 'namespaceliarsdice_1_1logging'),
    ]),
]

# Class documentation
LINKS_NAVBAR2 = [
    ("Classes", 'annotated', [
        ('Core Classes', 'classliarsdice_1_1core_1_1_game'),
        ('AI Classes', 'classliarsdice_1_1ai_1_1_i_a_i_strategy'),
        ('Data Structures', 'classliarsdice_1_1data__structures_1_1_trie_map'),
    ]),
    ("Files", 'files', [
        ('Headers', 'dir_d44c64559bbebec7f509842c48db8b23'),
        ('Sources', 'dir_68267d1309a1af8e8297ef4c3efb3e8a'),
    ]),
]

# m.css theme configuration
M_THEME_COLOR = '#22272e'

# Project information
PROJECT_NAME = "Liar's Dice"
PROJECT_BRIEF = "A modern C++20 implementation of the classic dice game with AI players"

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
