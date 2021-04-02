`*.lua` files from this dir are also used to generate html documentation.

After every change run the following commands to update it:

```
luadocumentor -f doc -d ../../../docs/source/generated-luadoc/lua-api-reference *.lua
sed -i 's/openmw\.\(\w*\)\(\#\|\.html\)/\1\2/g' ../../../docs/source/generated-luadoc/lua-api-reference/*.html
```

`luadocumentor` can be installed via `luarocks`:

```
luarocks install luadocumentor
```
