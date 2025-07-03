from docutils import nodes
from docutils.parsers.rst import Directive, directives

class OMWSettingDirective(Directive):
    has_content = True
    option_spec = {
        'title': directives.unchanged_required,
        'type': directives.unchanged_required,
        'range': directives.unchanged,
        'default': directives.unchanged,
        'location': directives.unchanged,
    }

    badge_map = {
        'float32': 'bdg-secondary', 'float64': 'bdg-muted',
        'int': 'bdg-primary', 'uint': 'bdg-light',
        'string': 'bdg-success', 'boolean': 'bdg-warning',
        'color': 'bdg-info',
    }

    def run(self):
        opts = self.options
        title = opts['title']
        type_tags = opts['type'].split('|')
        badges = ' '.join(f':{self.badge_map.get(t)}:`{t}`' for t in type_tags)

        values = [
            badges,
            opts.get('range', ''),
            opts.get('default', ''),
            opts.get('location', ':bdg-danger:`user settings.cfg`')
        ]

        table = nodes.table(classes=['omw-setting'])
        tgroup = nodes.tgroup(cols=4)
        table += tgroup
        for _ in range(4):
            tgroup += nodes.colspec(colwidth=20)

        thead = nodes.thead()
        tgroup += thead
        thead += nodes.row('', *[
            nodes.entry('', nodes.paragraph(text=label))
            for label in ['Type', 'Range', 'Default', 'Location']
        ])

        tbody = nodes.tbody()
        tgroup += tbody
        row = nodes.row()

        for i, val in enumerate(values):
            entry = nodes.entry()
            inline, _ = self.state.inline_text(val, self.lineno)
            if i == 2 and 'color' in type_tags:
                rgba = [float(c) for c in opts['default'].split()]
                style = f'background-color:rgba({rgba[0]*255}, {rgba[1]*255}, {rgba[2]*255}, {rgba[3]})'
                chip = nodes.raw('', f'<span class="color-chip" style="{style}"></span>', format='html')
                wrapper = nodes.container(classes=['type-color-wrapper'], children=[chip] + inline)
                entry += wrapper
            else:
                entry += inline
            row += entry
        tbody += row

        desc = nodes.paragraph()
        self.state.nested_parse(self.content, self.content_offset, desc)

        section = nodes.section(ids=[nodes.make_id(title)])
        section += nodes.title(text=title)
        section += table
        section += desc
        return [section]

def setup(app):
    app.add_css_file("omw-directives.css")
    app.add_directive("omw-setting", OMWSettingDirective)
