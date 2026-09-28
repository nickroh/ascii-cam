1. Visual Studio 설치
    * CPP을 이용한 데스크톱앱 어쩌고

2. github https://github.com/whereisjaeyoon/ascii-cam

3. build  -> project -> ... properties
    * dll release/debug -> conf type : dll
    * opencv: C/C++/General/Additional Include Dir -> <path>\opencv\build\include;%(AdditionalIncludeDirectories)
    * opencv: Linker/General -> <path>\opencv\build\x64\vc16\lib;%(AdditionalLibraryDirectories)
    * https://m.blog.naver.com/damtaja/223427382026

4. 윈도우 + r -> regedit
```cpp
const wchar_t* clsid =
L"{3CAD447D-F283-4AF4-A3B2-6F5363309F52}";
```
컴퓨터\HKEY_CLASSES_ROOT\CLSID\{3CAD447D-F283-4AF4-A3B2-6F5363309F52}\InprocServer32

```
Windows에서 C++로 만든 COM DLL을 레지스트리에 수동으로 등록하려고 합니다.

DLL의 CLSID는 다음과 같습니다.

{3CAD447D-F283-4AF4-A3B2-6F5363309F52}

DLL의 절대 경로는 이미 알고 있습니다.

C:\path\to\MyFilter.dll

regsvr32 명령어를 사용하는 방법이 아니라 Windows 레지스트리 편집기(regedit)를 GUI로 사용해서 직접 등록하는 방법​을 알려주세요.

특히 다음을 정확히 설명해주세요.

HKEY_CLASSES_ROOT에서 어떤 키를 만들어야 하는지
CLSID 키 아래에 어떤 하위 키를 만들어야 하는지
(기본값)에 DLL 경로를 어떻게 넣는지
ThreadingModel은 어떤 종류의 값으로 만들고 어떤 값을 넣는지
최종 레지스트리 구조를 트리 형태로 보여주세요.

예를 들어 다음 C++ 코드에서 사용하는 CLSID입니다.

const wchar_t* clsid =
    L"{3CAD447D-F283-4AF4-A3B2-6F5363309F52}";

이 DLL은 Windows COM/Media Foundation 관련 DLL입니다. 단순한 설명이 아니라 regedit에서 실제로 입력해야 하는 값과 위치를 단계별로 알려주세요.
```

5. x64 Native Tools Command Prompt 어쩌고 -> 관리자 권한으로 실행
    * `net stop FrameServer`
    * `net start FrameServer`