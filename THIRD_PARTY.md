# 서드파티 에셋 출처

이 저장소의 **소스 코드**는 MIT 라이선스([LICENSE](LICENSE))를 따름. 여기 적힌 에셋은 그렇지 않음.

주의할 점이 하나 있음. 아래 에셋의 출처인 `OtemPsych/Age-of-War` 저장소는 **2026-10-05 기준 LICENSE 파일이 없고, GitHub API로도 라이선스가 선언되어 있지 않음.** 라이선스가 없으면 redistribute 권한이 명시되지 않은 것이고, 공개 저장소에 있다는 이유만으로 redistribute가 허용되는 건 아니라고 보는 게 맞음.

그 상태로 이미 변환된 파일이 커밋되어 있음. 현재 저장소 상태의 서술임, 적법하다는 뜻이 아님. 이 건을 어떻게 정리할지는 이슈로 열려 있음.

## 출처가 `OtemPsych/Age-of-War`인 파일

3DS용으로 축소·변환함.

| 경로 | 원본 |
| --- | --- |
| `3ds/gfx/background.png` (+`.t3s`) | 배경 |
| `3ds/gfx/base.png` (+`.t3s`) | 기지 |
| `3ds/gfx/unit_bg.png` (+`.t3s`) | 유닛 카드 배경 |
| `3ds/gfx/smoke.png` (+`.t3s`) | 연기 |
| `3ds/data/aow3ds_font.bcfnt` | 폰트 (BCFNT로 변환) |

출처 저장소: <https://github.com/OtemPsych/Age-of-War>

## 원작 리소스 (요청으로 추가)

이번 변경에서 사용자가 원작 리소스를 그대로 써도 된다고 명시적으로 허락해서 아래 파일들을 넣음. 권리가 정리됐다는 뜻은 아님.

| 경로 | 원본 |
| --- | --- |
| `reference/flash-decompiled/**` | 플래시 원작 디컴파일 결과 전체. 원본 SWF 포함 |
| `3ds/gfx/aow/*.png`, `3ds/gfx/aow.t3s`, `3ds/gfx/aow.t3x` | 원작 유닛·기지·포탑·UI·이펙트 스프라이트를 3DS용으로 리사이즈/아틀라스화 |
| `3ds/data/*.pcm` | 원작 음악/효과음을 22050 Hz mono PCM16으로 변환 |
| `3ds/source/art_indices.h` | 위 아틀라스의 인덱스 정의(자동 생성) |

디컴파일 출처: <https://github.com/erupturatis/Decompiled-flash-games-archive> (Age of War 폴더)

스프라이트/음향 출처: <https://github.com/apiotrowski255/age-of-war> (`age of war sprites` 폴더)

원본 SWF: archive.org의 `flash_age_of_war` 항목에서 받음.

이 저장소의 3DS 코드는 원작 코드를 옮긴 게 아니라 디컴파일된 수치와 동작을 참고해 다시 구현한 것임. `reference/`는 대조용 원본 보관임.

## 출처가 확인되지 않은 파일

`pygame/Assets/` 아래는 원본 출처가 이 저장소 어디에도 기록되어 있지 않음. README에는 "원작 음악과 효과음은 아직 연결하지 않았다"고만 되어 있음.

| 경로 | 상태 |
| --- | --- |
| `pygame/Assets/Music/Soundtrack.ogg` | 출처 미상 |
| `pygame/Assets/Sounds/KnightAttack.ogg` | 출처 미상 |
| `pygame/Assets/Sounds/MageAttack.ogg` | 출처 미상 |
| `pygame/Assets/Fonts/*.ttf` | 출처 미상 |
| `pygame/Assets/Textures/*.png`, `*.jpg` | 출처 미상 |
| `pygame/Assets/TextureData/*.json` | 출처 미상 |

## 기여자 유의사항

- **서드파티 에셋을 새로 커밋하지 말 것.** 라이선스가 확실한 것만. 확실하지 않으면 이슈를 먼저 열어서 물어봄. 위 원작 리소스는 사용자가 명시적으로 허락한 예외임.
- 게임에 실제로 쓰이는 에셋(`3ds/gfx/`, `3ds/data/`)을 갈아타는 건 이슈에서 먼저 논의함. 화면 크기·색상·폰트 폭이 코드에 박혀 있어서 에셋만 바꾸면 UI가 깨질 수 있음.
- 원작 에셋의 권리 상태가 정리되면 이 문서를 그때 갱신함.

English summary: assets under `3ds/gfx/`, `3ds/data/` come from `OtemPsych/Age-of-War` and original game resources in `apiotrowski255/age-of-war`; `reference/flash-decompiled/` is a full decompilation from `erupturatis/Decompiled-flash-games-archive`. The user explicitly approved adding these original resources. None of them are covered by this repo's MIT license, and their rights are unresolved. Assets under `pygame/Assets/` have undocumented provenance. Don't commit new third-party assets without an issue first, unless the user explicitly says otherwise.
