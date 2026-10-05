# 기여 안내

기여해 줘서 고맙습니다. 이슈·PR 둘 다 영어로 써도 되고 한국어로 써도 됩니다.

## 돌아가는 것, 안 돌아가는 것

한 줄로 정리하면 이 저장소엔 **동작하는 3DS 빌드 하나**만 있고 나머지는 진행 중이다.

| 경로 | 상태 |
| --- | --- |
| `3ds/` | libctru(C++) 포트. 실제로 동작함. 빌드 가능 |
| `3ds/source/` | 게임 규칙 + 렌더링 + 입력 |
| `3ds/gfx/`, `3ds/data/` | 변환된 에셋. 서드파티 권리 문제 있음 → [THIRD_PARTY.md](THIRD_PARTY.md) |
| `pygame/` | Python 재구현. 데이터 테이블만 옮겨둔 상태. **진입점이 없어서 실행 안 됨** |
| `install/` | FBI 원격 설치용 QR 코드 + 스캔용 HTML |

그래서 PR 주제가 이 둘 중 하나가 됨:

1. `3ds/`의 게임 로직·UI·버그 수정
2. `pygame/` 구현 이어가기 (엔트리포인트 만들기 포함)

## 준비물

빌드하려면 devkitPro의 3DS 환경이 필요함.

```
devkitARM
libctru, libcitro2d, libcitro3d
tools: makerom, bannertool
```

설치 후 `DEVKITPRO`만 맞춰주면 됨. `3ds/env.sh`가 나머지 경로를 알아서 채움.

Windows는 WSL에서 하는 게 편함. macOS·Linux는 네이티브로 됨.

```sh
export DEVKITPRO="$HOME/devkitpro"   # 설치 위치가 다르면 이거만 바꿔줌
```

## 빌드

```sh
cd 3ds
source env.sh
make clean
make          # aow3ds.3dsx
./build-cia.sh   # aow3ds.3dsx + aow3ds.cia + banner.bnr
```

산출물은 `3ds/` 아래에 생김. 전부 `.gitignore`에 있음. **커밋하지 말 것.**

`make clean`이 안 지우는 게 `aow3ds.rsf`랑 `banner.bnr`. 필요하면 직접 지움.

## 실행해서 확인하기

3DS 하드웨어가 필요함. 가뭄 에뮬레이터로 확인할 건 소스 빌드 수준까지임.

하드웨어가 있으면:

1. 시드 권한 있는 커스텀 펌웨어가 깔린 3DS에 CIA를 넣음
2. `aow3ds.cia`를 SD 카드 아무 데나 복사
3. FBI → `SD` → 파일 고르기 → `Install and delete CIA`
4. 홈 메뉴에서 Age of War 3DS 실행

Unique ID가 `0x0003FF3F`(개발용)이라서 시그패치가 있는 커스텀 펌웨어에서만 설치됨. 시드만 있는 기기에서는 설치 자체가 안 됨. 이건 고칠 문제가 아니라 지금 설정값임.

조작법은 [`3ds/README.md`](3ds/README.md)에 정리돼 있음.

## 코드 구조

```
3ds/source/data.h      상수, 밸런스 테이블, 화면 레이아웃 좌표
3ds/source/sim.h/.cpp  게임 규칙. 렌더링 없음, 상태만 갱신
3ds/source/main.cpp    citro2d 렌더링, 터치/버튼 입력, UI
```

`sim.cpp`가 규칙이고 `main.cpp`가 그려주는 쪽이라 둘은 최대한 섞지 않는 게 좋음. 게임 규칙을 건드린다면 3DS에서 직접 확인해볼 수 있으면 그게 제일 확실함.

텍스처는 `gfx/*.t3s`를 picasso로 굽고 `bin2o`로 링크함. 새 이미지 넣으려면 `.t3s`도 같이 갱신해야 함.

## 원작에 더 가깝게 만들기

이 저장소가 원작과 다른 채로 있는 게 사실임. 화면 배치와 밸런스가 꽤 다르고 다 완성된 건 아님. 그래도 시작부터 끝까지 플레이는 됨.

그래서 여기가 기여할 자리. 밸런스 데이터(`3ds/source/data.h`)는 원작 기준표가 이미 들어있고 UI도 유닛 16종·포탑 15종·시대 5단계까지 전부 띄우고 있음. 원작의 뼈대는 대체로 맞춰져 있고 빠진 건 아래 것들.

### Making it closer to the original

*English summary of this section. The details below are in Korean.*

This port genuinely isn't the original. The layout and balance are noticeably off and it isn't finished, though it does play through. So this is where contributions go.

The balance tables in `3ds/source/data.h` are already the original's, and the UI shows all of them — 16 unit types, 15 turret types, 5 ages. Most of the original's structure is in place. What's missing:

- **Difficulty selection.** The rules exist (`simInit(s, difficulty)` scales resources ×1.3 / ×2.0) but `main.cpp` hardcodes `g_difficulty = 0`. Needs a menu and a real source for that value.
- **Saving / replays.** Never touches `repo`, and the CIA sets `SaveDataSize: 0K`. Needs `svch`/`fs` writes to the SD card, so the CIA save size has to grow first. Open an issue before starting.
- **Music and sound effects.** No audio code in `3ds/source/` at all. libctru uses `ndsp` + `dsp`; see the `silence.wav` bit of `build-cia.sh` for the format.
- **Turret upgrades.** Only slot expansion exists. `TURRET_DEFS` already has 15 entries, so it's the upgrade path that's missing.
- **The `pygame/` side.** Data tables only, no scenes. Writing `main.py` (menu → battle → victory/defeat) is the first task there.

Easiest first PR is the difficulty menu — small and self-contained. If you don't know the original well, pointing out where the numbers in `data.h` don't match what you remember is genuinely useful.

### 빠져 있는 것

**난이도 선택**

규칙은 다 있음. `simInit(s, difficulty)`가 받는 값으로 자원을 1.3배(harder) 또는 2.0배(impossible) 깎고 실패 메시지도 다르게 나옴. 근데 `main.cpp`의 `g_difficulty = 0`이 하드코딩이라 플레이어가 고를 수단이 없음.

따라 할 일은 두 가지. 시작 화면에 난이도 고르는 UI 만들고 `g_difficulty`를 거기서 받도록 바꾸는 것. 가장 작고 독립적이라 처음 Contributor 해보기 괜찮음.

**세이브 / 리플레이**

원작은 리플레이 시스템이 있고 진행 상황이 남음. 이쪽은 repo를 아예 안 씀. CIA 설정도 `build-cia.sh`에서 `SaveDataSize: 0K`라 저장 공간이 없음.

libctru로 SD 카드에 직접 저장하려면 `svch`/`fs`를 써야 하고, 그러려면 CIA의 `SaveDataSize`를 늘려야 함. 설계가 좀 갈라서 이슈를 먼저 여는 게 나음.

**음악과 효과음**

`3ds/source/`에 오디오 코드가 아예 없음. libctru는 `ndsp` + `dsp`로 재생함. `build-cia.sh`가 `silence.wav`를 만들어 쓰고 있으니 포맷은 거기서 참고. 원작 음악은 용량이 커서 우선 배경음만 넣는 게 현실적일 듯.

에셋은 출처를 밝혀야 하니까 서드파티 음원 쓰려면 이슈를 먼저 열 것. ([THIRD_PARTY.md](THIRD_PARTY.md) 참고)

**포탑 업그레이드**

지금은 `simAddExpansion`으로 슬롯만 늘어나고 업그레이드는 없음. 원작엔 포탑 레벨업과 회전이 있음. `TURRET_DEFS`에 15종이 이미 들어있으니까 업그레이드 경로만 설계하면 됨.

**`pygame/` 쪽**

`src/settings.py`랑 `src/gamedata.py`에 데이터 테이블만 있고 화면도 입력도 시뮬도 없음. 메뉴 → 전투 → 승리/패배를 잇는 진입점(`main.py`) 만드는 게 첫 과제. 원작을 옮기기보다 3DS 판을 베끼는 게 빠름.

### 어디서부터 손대면 좋은가

- **처음이라 3DS 빌드부터 익히고 싶음** → 문서 오타, 죽은 코드 제거 같은 것. 하드웨어 없이 빌드만으로 끝나서 안전함
- **난이도 선택** → 위에서 말한 UI 추가. 작고 독립적이라 첫 PR로 좋음
- **원작을 잘 아님** → `data.h`가 원작 기준이라, 원작 숫자와 다른 데가 보이면 이슈로 알려주면 좋음. 대조해서 틀린 부분 짚어주는 것도 기여임
- **원작을 아주 잘 암** → 빠진 기능 순서대로. 첫 PR은 위 목록에서 하나만 고르는 게 나음. 세이브나 오디오 같은 큰 덩어리는 한 번에 다 하려 하면 리뷰가 빡세짐

이쪽이 원작을 모르니까 뭐가 급한지는 기여자가 알려주는 거임. 다른 부분을 짚어주면 이슈로 남겨줘.

## 폰트

`3ds/data/aow3ds_font.bcfnt`를 만든 스크립트가 이 저장소에 없음. 다시 만들려면 별도 BCFNT 변환 툴이 필요함.

그래서 폰트 바꾸는 PR은 받기 어려움. 먼저 이슈를 열어서 방법을 같이 정할 것.

## 브랜치와 커밋

브랜치 이름:

- 버그 수정 → `fix/issue-<번호>`
- 기능 추가 → `feat/<짧은-요약>`

커밋 메시지는 앞에 타입을 붙이는 걸 쓰면 읽기 편함. 저장소에 이미 있는 관례임.

```
docs: README 언어 링크 추가
build: 3dsx와 cia 정리
sim: 포탑 슬롯 확장 쿨다운 적용
```

길게 쓸 필요는 없고 왜 바꿨는지만 분명히.

## PR 올리는 법

**버그 수정이면 이슈를 먼저 열 것.** 원인이 뭔지 파면서 진행 상황을 이슈 댓글로 남기고, 그 이슈를 닫으면서 PR을 머지함.

**기능 추가면 이슈 없이 PR만.** 요구가 완성된 상태로 넘어온 경우니까.

PR 본문에 `Closes #<번호>` 넣으면 머지할 때 이슈가 자동으로 닫힘.

PR 올리기 전 체크:

- [ ] 빌드해서 확인했음 (`make clean && make && ./build-cia.sh` 통과)
- [ ] 빌드 산출물 커밋 안 함
- [ ] 새 서드파티 에셋 안 넣었음, 넣었으면 이슈에서 먼저 얘기함
- [ ] 텍스트 파일에 깨진 인코딩이나 제어문자가 없음 (아래 참고)

## 인코딩 주의

**텍스트 파일에 제어문자를 넣지 말 것.**

실제로 이 저장소에서 이 사고가 났음. `README.en.md`에 백틱 자리에 `0x07`(BEL), 숫자 `0` 자리에 `0x00`(NUL)이 들어가서 코드펜스가 통째로 날아갔고 GitHub 렌더링이 깨졌음. 파일은 UTF-8 무결성 검사를 통과하므로看不出来.

- 텍스트 파일은 UTF-8, 줄바꿈은 LF. BOM 없음
- 커밋 전에 이상한 문자가 없는지 확인:

```sh
LC_ALL=C grep -nP '[\x00-\x08\x0b\x0c\x0e-\x1f]' README.md README.en.md 3ds/README.md
```

출력 없어야 정상. 나왔으면 그 바이트를 지우고 다시.

릴리스 노트도 같은 문제로 망가진 적이 있음. 백틱이랑 `→`를 넣을 때 주의.

## 질문

확신이 안 서는 건 이슈로 물어보는 게 낫음. 디스코드나 슬랙 같은 거 없음. 이 저장소가 작아서 질문이 곧 문서가 됨.
