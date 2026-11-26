[English README](README.en.md) | [한국어 README](README.md)

# Age of War 3DS

Age of War를 닌텐도 3DS(카트리지와 devkitARM)용으로 다시 만든 2D 디펜스 게임.
C++/libctru로 3DS에 포트했고, 같은 요소를 파이썬(pygame)으로 옮긴 작업 폴더도 같이 둔다.

> [!NOTE]
> **개인적으로 만든 비공식 팬 포트임.** 원작 개발사나 배포사와 관계가 없고, 원작 이름은 출처 표기용으로만 씀.
>
> 화면 구성과 밸런스를 원작 기준으로 맞추고 있음. 그래도 아직 완성은 아니고, 실기기 검증도 안 끝났음. 시작부터 끝까지 플레이는 가능함.
>
> 원작에 더 가깝게 만들고 싶으면 [기여 안내](CONTRIBUTING.md)를 봄. 남은 건 아래 [원작과의 차이](#원작과의-차이)에 정리해둠.

## 원작과의 차이

밸런스 데이터(`3ds/source/data.h`)는 디컴파일한 플래시 원작 기준표를 그대로 넣었음 — 유닛 16종, 포탑 15종, 시대 5단계, 확장 비용 1000/3000/7500, 나이 업그레이드 경험치 500/4000/14000/45000/200000. 유닛·기지·포탑·UI·이펙트 스프라이트는 원작 리소스를 3DS 해상도에 맞춰 변환했고, 음악과 효과음도 `ndsp`로 연결함. 난이도는 원작 이름 그대로 타이틀에서 Normal/Harder/Impossible 중 고를 수 있고, 효과는 원작 배율(1.3/2.0)을 씀. 타이틀의 HOW TO PLAY에서 원작 설명 요약도 볼 수 있음. 포탑은 배치·판매·슬롯 확장이 되고, 시대별 5종 특수기도 넣음.

디컴파일 원본은 [`reference/flash-decompiled/`](reference/flash-decompiled/)에 그대로 들어 있음. 출처와 권리 상태는 [`THIRD_PARTY.md`](THIRD_PARTY.md)에 적었음.

아직 남은 것:

- 세이브 / 리플레이. 저장소를 아예 안 쓰고 CIA 설정도 `SaveDataSize: 0K`라 진행 기록이 남지 않음
- 포탑 업그레이드. 배치·판매·슬롯 확장까지만 있고, 개별 포탑 업그레이드는 아직 없음
- `pygame/` 쪽. 데이터 테이블만 옮겨둔 상태라 실행 자체가 안 됨
- 실기기 검증. 에뮬레이터/빌드까지만 확인함

원작을 직접 아는 사람이 여기 빠진 걸 채워주면 좋음. 어디부터 손대면 좋은지는 [기여 안내](CONTRIBUTING.md#원작에-더-가깝게-만들기)에 있음.

## 설치

릴리스에 미리 빌드된 파일이 있다.

| 파일 | 설명 |
| --- | --- |
| `aow3ds.cia` | 커스텀 펌웨어(Luma3DS 등)에서 설치할 CIA |
| `aow3ds.3dsx` | Homebrew Launcher용 |

### 방법 1: QR 코드 (FBI)

1. FBI를 실행한다.
2. `Remote Install` → `Scan QR code`.
3. 저장소 루트의 `install/aow3ds-qr.png`를 화면에 띄운다(흰 배경 유지, 가급적 크게).
4. 3DS 카메라로 읽으면 URL이 뜨고, `Yes`를 누르면 내려받아 설치한다.

QR에는 CIA의 직접 다운로드 링크가 들어간다. 네트워크가 되는 곳이면 SD 카드 없이 끝난다.

### 방법 2: SD 카드

`aow3ds.cia`를 SD 카드의 아무 곳이나 넣고 FBI → `SD` → 파일 선택 → `Install and delete CIA`.

## 빌드 (3DS)

devkitPro의 3DS 환경(`devkitARM`, `libctru`, `citro2d`)이 필요하다. WSL이나 리눅스/맥에서 진행.

```sh
cd 3ds
source env.sh
make clean
make
./build-cia.sh
```

`build-cia.sh`는 `makerom`으로 CIA, `bannertool`으로 배너를 만든다.
Unique ID는 `0x0003FF3F`(개발용 title ID)라서 시드 권한이 없어서 시그패치가 있는 커스텀 펌웨어에서만 설치된다.

자세한 조작법과 빌드 설명은 [`3ds/README.md`](3ds/README.md).

## 폴더 구조

```
3ds/                    libctru(C++) 포트. 지금 실제로 돌아가는 버전
   gfx/aow/             3DS용으로 줄인 원작 스프라이트
   data/*.pcm           원작 음악/효과음(22050 Hz mono PCM16)
   source/audio.cpp     ndsp 재생 코드
reference/              플래시 원작 디컴파일 원본(SWF, .as, PNG, MP3 포함)
pygame/                 파이썬 재구현. 데이터 테이블까지만 옮겨둔 상태(진행 중)
install/                QR 코드 PNG + 스캔용 HTML
tools/build-assets.ps1  원작 리소스를 3DS용 t3x/PCM으로 바꾸는 스크립트
```

## 에셋 출처

원작 리소스는 세 갈래로 들어왔다.

- 배경/기지/유닛 카드 배경과 폰트: [`OtemPsych/Age-of-War`](https://github.com/OtemPsych/Age-of-War) 공개 릴리스 에셋을 3DS용으로 축소·변환.
- 유닛·포탑·UI·이펙트 스프라이트와 음악/효과음: [`apiotrowski255/age-of-war`](https://github.com/apiotrowski255/age-of-war)에 들어 있는 원작 리소스를 3DS용으로 변환.
- 플래시 원작 디컴파일 결과: [`erupturatis/Decompiled-flash-games-archive`](https://github.com/erupturatis/Decompiled-flash-games-archive)의 Age of War 폴더를 `reference/flash-decompiled/`에 그대로 넣음. 원본 SWF도 포함.

이 소스들에는 라이선스가 없거나 권리 상태가 불분명하다. 그래서 이 에셋은 이 저장소 MIT 라이선스 대상이 아니며, 권리 상태가 정리되지 않은 상태로 커밋되어 있다. 자세한 건 [`THIRD_PARTY.md`](THIRD_PARTY.md).

`3ds/data/aow3ds_font.bcfnt`의 생성 스크립트는 저장소에 없다. 다시 만들려면 BCFNT 변환 툴이 따로 필요하다.

`tools/build-assets.ps1`은 원작 리소스 체크아웃을 `-Source`로 받아 `3ds/gfx/aow/`, `3ds/data/*.pcm`, `3ds/source/art_indices.h`를 다시 만든다. `magick`과 `ffmpeg`가 필요하다. 변환 결과는 커밋해두니까 보통은 안 돌려도 됨.

이 저장소의 3DS 코드는 원작 게임 코드를 옮긴 게 아니라 독립 구현이다. `reference/`는 원작 동작을 대조하기 위한 참고 자료로 넣어둔 것뿐이다. 빌드 산출물은 릴리스에만 올리고 커밋하지 않는다.

## 상태

- 3DS: 상단 화면에 원작 스프라이트 전투, 아래 화면에 기지 HP/자원/시대/생산 UI. 유닛 16종, 포탑 15종, 시대 5단계, 난이도 3단계, 시대별 특수기와 원작 사운드. 끝까지 플레이 가능.
- 파이썬 쪽: `src/settings.py`, `src/gamedata.py`와 에셋만 있는 상태. 실행 진입점(`main.py`)이 없어 돌아가지 않는다.

## 기여

이슈·PR 둘 다 영어든 한국어든 상관없음.

- 버그 → 이슈 먼저 열고, `fix/issue-<번호>` 브랜치에 고쳐서 PR. 본문에 `Closes #<번호>`
- 기능 → 이슈 없이 `feat/<요약>` 브랜치에 바로 PR
- 빌드 방법, 실행 방법, 브랜치/커밋 관례, 인코딩 주의사항 → [`CONTRIBUTING.md`](CONTRIBUTING.md)
- 질문이 애매하면 이슈로 물어보는 게 빠름. 이 저장소가 작아서 질문이 곧 문서가 됨

서드파티 에셋은 라이선스가 확실한 것만 커밋해달라. 애매하면 이슈를 먼저 열 것.

## 라이선스

소스 코드는 MIT ([`LICENSE`](LICENSE)). 서드파티 에셋은 해당 없음, [`THIRD_PARTY.md`](THIRD_PARTY.md) 참고.
