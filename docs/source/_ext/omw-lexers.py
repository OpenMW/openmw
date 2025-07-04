from pygments.lexer import RegexLexer, bygroups
from pygments.token import Comment, Name, Operator, String, Text
from sphinx.highlighting import lexers

class OMWConfigLexer(RegexLexer):
    name = 'openmwcfg'
    aliases = ['openmwcfg']
    filenames = ['openmw.cfg']

    tokens = {
        'root': [
            (r'(\s*)(#.*$)', bygroups(Text.Whitespace, Comment.Single)),
            (r'(\s*)([a-zA-Z0-9_.+-]+)(\s*(\+)?=\s*)(.*)',  bygroups(Text.Whitespace, Name.Attribute, Operator, Operator, String)),
            (r'.+\n', Text),
        ],
    }

def setup(_):
    lexers["openmwcfg"] = OMWConfigLexer()
