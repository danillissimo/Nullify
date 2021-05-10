# Nullify
```
Erases all files' time attributes: creation time, last access time, and last write time

nullify /p[ath] file_or_folder_path | /h[ere] | /i[nplace]
        /r[ecursive] [/e[xcluderoot]]
        /l[ist]
        /? /h[elp]

        Parameter case and order doesn't matter

Necessary parameters:
    /p[ath]             - must be followed by target folder/file name
    /h[ere], /i[nplace] - uses current working directory as path
If neither parameter is specified, nothing happens


Optional parameters:
    /r[ecursive]   - if target is a folder, recursively process all subfolders,
                     ignored otherwise
    /e[xcluderoot] - if target is a folder and parameter /r is present,
                     exclude target folder, ignored otherwise
    /l[ist]        - list all processed files and folders, increases execution
                     time
    /?, /help      - if present, all other params are ignored and this tip
                     is shown
```
