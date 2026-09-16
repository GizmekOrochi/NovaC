from pathlib import Path

DOC_CONFIG = Path(__file__).resolve().parent
PROJECT_ROOT = DOC_CONFIG.parent.parent

project = "NovaC"
author = "GizmekOrochi"
copyright = "2026, GizmekOrochi"
version = "1.0"
release = "1.0"

extensions = [
    "breathe",
    "sphinx.ext.autosectionlabel",
    "sphinx.ext.todo",
    "sphinx_copybutton",
]

autosectionlabel_prefix_document = True
primary_domain = "cpp"
highlight_language = "cpp"
todo_include_todos = True

breathe_projects = {
    "NovaC": str(DOC_CONFIG / "_build" / "doxygen" / "xml"),
}
breathe_default_project = "NovaC"
breathe_domain_by_extension = {"h": "cpp", "hpp": "cpp"}

source_suffix = {
    ".rst": "restructuredtext",
}
master_doc = "index"
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "furo"
html_title = "NovaC Documentation"
html_short_title = "NovaC"
html_static_path = ["source/_static"]
html_css_files = ["custom.css"]
html_theme_options = {
    "sidebar_hide_name": False,
    "navigation_with_keys": True,
    "top_of_page_button": "edit",
}

copybutton_prompt_text = r">>> |\.\.\. |\$ |# "
copybutton_prompt_is_regexp = True


nitpicky = False
