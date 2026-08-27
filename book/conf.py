# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'X65'
copyright = '2025, Tomasz Sterna'
author = 'Tomasz Sterna'

html_title = "X65.docs"
html_logo = "_static/paw_logo_32.png"
html_favicon = '_static/paw_logo_32_gray.png'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.todo',     # To-do list support
    'sphinx.ext.mathjax',  # Math support
    'sphinx_sitemap',
    'myst_parser',
    'sphinx_design',
    'sphinxext.opengraph',
    'sphinx_inline_tabs',
    'sphinx_copybutton',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'AGENTS.md']

todo_include_todos = True

html_js_files = [
    'https://plausible.io/js/script.js',
]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'
html_static_path = ['_static']
html_theme_options = {
    'source_repository': 'https://github.com/X65/X65/',
    'source_branch': 'main',
    'source_directory': 'book/',
    # "sidebar_hide_name": True,
    "light_css_variables": {
        "color-brand-primary": "#0E556F",
        "color-brand-content": "#0E556F",
        "color-brand-visited": "#052544",
    },
    "dark_css_variables": {
        "color-brand-primary": "#90D5F1",
        "color-brand-content": "#90D5F1",
        "color-brand-visited": "#4586A0",
    },
}
html_baseurl = 'https://docs.x65.zone/' # sphinx-sitemap
html_css_files = [
    'custom.css',
]

sitemap_url_scheme = "{link}"

myst_enable_extensions = ['colon_fence']
# Generate anchors for headings down to level 4 so `file.md#some-heading` links
# between chapters resolve; without this every such cross-reference warns.
myst_heading_anchors = 4

# Every ```asm block in this book is ca65 (the cc65 assembler). Pygments' default
# `asm` lexer is GNU as, which fails on ca65's `.struct`, `::` and `|` syntax and
# makes Sphinx fall back to relaxed lexing with a warning. Point `asm` at ca65.
from pygments.lexers import get_lexer_by_name  # noqa: E402
from sphinx.highlighting import lexers  # noqa: E402

lexers['asm'] = get_lexer_by_name('ca65')

ogp_site_url = 'https://docs.x65.zone'

pygments_style = "sphinx"
pygments_dark_style = "monokai"
