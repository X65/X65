# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'X65'
copyright = '2025, Tomasz Sterna'
author = 'Tomasz Sterna'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['breathe',
    'sphinx_sitemap',
    'myst_parser',
    'sphinx_design',
    'sphinxext.opengraph',
    'sphinx_inline_tabs',
    'sphinx_copybutton',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'
html_static_path = ['_static']
html_theme_options = {
    # 'source_repository': 'https://github.com/X65/X65/',
    # 'source_branch': 'main',
    # 'source_directory': 'docs/',
}
html_baseurl = 'docs.x65.zone' # sphinx-sitemap

# Breathe Configuration
breathe_default_project = 'X65'
breathe_default_members = ('members', 'undoc-members')

myst_enable_extensions = ['colon_fence']

ogp_site_url = 'http://docs.x65.zone'
