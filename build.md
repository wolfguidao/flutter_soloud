| | Cocoapod | SPM |
|---|---|---|
|ios using **Podfile** with</br>**ENV['NO_OPUS_OGG_LIBS'] = '1'**|[x]|not possible|
|ios using **Podfile** without</br>**ENV['NO_OPUS_OGG_LIBS'] = '1'**|[x]|not possible|
|ios using **env** set </br>**export NO_OPUS_OGG_LIBS="1" && flutter run**|[x]|[x]|
|ios using **env not** set</br>**unset NO_OPUS_OGG_LIBS && flutter run**|[x]|[x]|

| | Cocoapod | SPM |
|---|---|---|
|macos using **Podfile** with</br>**ENV['NO_OPUS_OGG_LIBS'] = '1'**|[x]|not possible|
|macos using **Podfile**  without</br>**ENV['NO_OPUS_OGG_LIBS'] = '1'**|[x]|not possible|
|macos using **env** set</br>**export NO_OPUS_OGG_LIBS="1" && flutter run**|[x]|[x]|
|macos using **env not** set</br>**unset NO_OPUS_OGG_LIBS && flutter run**|[x]|[x]|

Before each tests I did this in macos or ios dirs:
```
rm -rf Pods Podfile.lock
pod install --repo-update
```

The issue is that SPM builds don't inherit environment variables from the Podfile, the Podfile belongs to CocoaPods.

Option 1: Set environment variable when building
cd /Volumes/NVME/workspace/libs/flutter_soloud/example
```
NO_OPUS_OGG_LIBS=1 flutter build macos
# or
NO_OPUS_OGG_LIBS=1 flutter run -d macos
```

Option 2: Modify Xcode Scheme (for development)
- Open macos/Runner.xcworkspace in Xcode
- Go to Product > Scheme > Edit Scheme
- Select Run in the left panel
- Go to the Arguments tab
- Add environment variable: NO_OPUS_OGG_LIBS = 1