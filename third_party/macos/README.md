# Syphon.framework (vendoré)

`Syphon.framework` est le framework macOS de partage vidéo inter-applications
([Syphon](https://github.com/Syphon/Syphon-Framework)), utilisé par la source
**Syphon** de MapMap. Licence **BSD 2-Clause** (voir `Syphon.LICENSE.txt`).

Le binaire est universel (`x86_64` + `arm64`) et son `install_name` est
`@rpath/Syphon.framework/Versions/A/Syphon`. À la compilation, `src/src.pri`
l'embarque dans `MapMap.app/Contents/Frameworks` et ajoute le `@rpath`
`@executable_path/../Frameworks`.

## Reconstruire le framework

Depuis une copie des sources de Syphon-Framework :

```sh
xcodebuild -project Syphon.xcodeproj -scheme Syphon -configuration Release \
  -derivedDataPath /tmp/syphon-build CODE_SIGNING_ALLOWED=NO \
  EXCLUDED_SOURCE_FILE_NAMES="SyphonMetalShaders.metal" build
cp -R /tmp/syphon-build/Build/Products/Release/Syphon.framework ./
```

> `EXCLUDED_SOURCE_FILE_NAMES="SyphonMetalShaders.metal"` évite la compilation
> du shader Metal, qui exige le « Metal Toolchain » (téléchargement Xcode
> séparé). MapMap n'utilise que le client **OpenGL** de Syphon, donc le shader
> Metal n'est pas nécessaire. Les fichiers `.m` Metal compilent normalement.
