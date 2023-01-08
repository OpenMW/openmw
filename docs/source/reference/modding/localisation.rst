Localisation
============

OpenMW supports localisation of mods using ICU MessageFormat wrapped in YAML.
Currently this is only possible using the
`openmw.core.l10n <../lua-scripting/openmw_core.html##(core).l10n>`_ lua function.

Locales
-------

Locales usually have the form ``{lang}_{COUNTRY}``,
where ``{lang}`` is a lowercase two-letter language code and ``{COUNTRY}`` is an uppercase
two-letter country code. Localisation files *must* have this exact capitalisation and separator
to be recognized.

When users request a locale using the :ref:`preferred locales` setting they do not need to match capitalisation
and can also use hyphens instead of underscores. The locale will be normalised to the above format.

Locales may also contain variants and keywords, though these usually will not be necessary.
See `The Locale chapter of the ICU Guide <https://unicode-org.github.io/icu/userguide/locale/#language-code>`_ for full details.

Fallbacks
---------

When OpenMW looks up messages at runtime, it starts with the first requested locale, and then looks at that locale's more generic ancestors before looking at the next requested locale. E.g. ``en_GB_OED`` will fall back to ``en_GB``, which will fall back to ``en``.

When including localisations with specific country variants (or more specific variants/keywords), you should always include the more generic version as well.

E.g. if you include ``en_US.yaml`` and ``en_GB.yaml`` localisation files, you should also include ``en.yaml``, since other English locales will fall back to that (e.g. ``en_CA``, ``en_AU``, ``en_NZ``). You can put an arbitrary ``en`` locale of your choice in ``en.yaml``, and then leave the file for that variant empty (since all lookups for the variant will fall back to ``en`` anyway).

Note that because of the fallbacks only messages which differ between variants need to be included in the country-specific localisation files.

Localisation Files
------------------

Localisation files (containing the message names and translations) should be stored in the
VFS as files of the form ``l10n/<ContextName>/<Locale>.yaml``.

**Naming policy**

"ContextName" should be in line with :ref:`Lua scripts naming policy`:

- L10n files for ``scripts/<ModName>/<ScriptName>.lua`` should be placed to ``l10n/<ModName>/<Locale>.yaml``.
- L10n files for ``scripts/<AuthorName>/<ModName>/<ScriptName>.lua`` should be placed to ``l10n/<AuthorName><ModName>/<Locale>.yaml``.

In most cases one mod should have only one l10n context. Don't create a new context for each single message. Really big mods with hundreds and thousands of messages can have several l10n contexts. In this case all context names should start with the name of the mod. I.e. ``<ContextName> = <ModName><Subcontext>`` (or ``<AuthorName><ModName><Subcontext>``).

L10n contexts with prefix "OMW" are reserved for the OpenMW itself (in particular for built-in scripts ``scripts/omw/``) and shouldn't be used in mods.

Built-in l10n contexts "Interface" and "Calendar" don't have the "OMW" prefix because these messages are more generic and can be reused in mods.

**Format**

Messages contents have the form of ICU MessageFormat strings.
See `the Formatting Messages chapter of the ICU Guide <https://unicode-org.github.io/icu/userguide/format_parse/messages/>`_
for a guide to MessageFormat, and see
`The ICU APIdoc <https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/classicu_1_1MessageFormat.html>`_
for full details of the MessageFormat syntax.

Examples
~~~~~~~~

.. code-block:: yaml
    :caption: DataFiles/l10n/MyMod/en.yaml

    good_morning: 'Good morning.'

    you_have_arrows: |-
      {count, plural,
        one {You have one arrow.}
        other {You have {count} arrows.}
      }

.. code-block:: yaml
    :caption: DataFiles/l10n/MyMod/de.yaml

    good_morning: "Guten Morgen."
    you_have_arrows: |-
      {count, plural,
        one {Du hast ein Pfeil.}
        other {Du hast {count} Pfeile.}
      }
    "Hello {name}!": "Hallo {name}!"

Select rules can be used to match arbitrary string arguments.
The default keyword ``other`` must always be provided.

.. code-block:: yaml
    :caption: DataFiles/l10n/AdvancedExample/en.yaml

    pc_must_come: {PCGender, select,
        male {He is}
        female {She is}
        other {They are}
      } coming with us.

Numbers have various formatting options and can also be formatted with custom patterns.
See `The ICU Guide <https://unicode-org.github.io/icu/userguide/format_parse/numbers/skeletons.html#syntax>`_

.. code-block:: yaml
    :caption: DataFiles/l10n/AdvancedExample2/en.yaml

    quest_completion: "The quest is {done, number, percent} complete."
    # E.g. "You came in 4th place"
    ordinal: "You came in {num, ordinal} place."
    # E.g. "There is one thing", "There are one hundred things"
    spellout: "There {num, plural, one{is {num, spellout} thing} other{are {num, spellout} things}}."
    numbers: "{int} and {double, number, integer} are integers, but {double} is a double"
    rounding: "{value, number, :: .00}"
