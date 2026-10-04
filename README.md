# Age of War 3DS

Age of War를 닌텐도 3DS(카트리지와 devkitARM)용으로 다시 만든 2D 디펜스 게임.
C++/libctru로 3DS에 포트했고, 같은 요소를 파이썬(pygame)으로 옮긴 작업 폴더도 같이 둔다.

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
3ds/       libctru(C++) 포트. 지금 실제로 돌아가는 버전
pygame/    파이썬 재구현. 데이터 테이블까지만 옮겨둔 상태(진행 중)
install/   QR 코드 PNG + 스캔용 HTML
```

## 에셋 출처

배경/기지/유닛 카드 배경과 폰트는 [`OtemPsych/Age-of-War`](https://github.com/OtemPsych/Age-of-War) 공개 릴리스 에셋을 3DS용으로 축소·변환한 것이다. 원작 음악과 효과음은 용량 때문에 아직 연결하지 않았다.

`3ds/data/aow3ds_font.bcfnt`의 생성 스크립트는 저장소에 없다. 다시 만들려면 BCFNT 변환 툴이 따로 필요하다.

이 저장소의 코드는 원작 게임 코드를 옮긴 게 아니라 독립 구현이다. 빌드 산출물은 릴리스에만 올리고 커밋하지 않는다.

## 상태

- 3DS: 상단 화면에 전투, 아래 화면에 기지 HP/자원/시대/생산 UI. 유닛 6종, 포탑 2종, 시대 3단계.
- 파이썬 쪽: `src/settings.py`, `src/gamedata.py`와 에셋만 있는 상태. 실행 진입점(`main.py`)이 없어 돌아가지 않는다.