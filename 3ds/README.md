# Age of War 3DS

Age of War의 Nintendo 3DS용 2D 디펜스 게임 포트입니다.

## 플레이

- 위 화면은 원작 스타일의 전투 화면입니다.
- 아래 화면은 기지 HP, 자원, 시대, 유닛/포탑 생산 UI입니다.
- 아래 화면의 카드 영역을 터치해 유닛을 생산합니다.
- 상단 `UNIT/TURR` 버튼으로 유닛 생산과 포탑 건설 메뉴를 전환합니다.
- `AGE`는 경험치가 충분할 때 시대를 발전시킵니다.
- `SPEC`은 특수 공격을 사용합니다.
- `SLOT`은 포탑 슬롯을 확장합니다.
- 전투 중 유닛을 터치하면 상태와 사거리를 확인할 수 있습니다.
- `SELECT`는 일시정지, `START`는 종료입니다.

## Linux에서 빌드

devkitPro의 3DS 개발 환경(`devkitARM`, `libctru`, `citro2d`)이 필요합니다.

```sh
source env.sh
make clean
make
./build-cia.sh
```

생성되는 파일:

- `aow3ds.3dsx`: Homebrew Launcher용
- `aow3ds.cia`: 커스텀 펌웨어 설치용

이 프로젝트는 시스템 폰트 대신 번들된 BCFNT 폰트를 사용하므로 콘솔 지역에 의존하지 않습니다.

## 원작 에셋

배경, 기지, 유닛 카드 배경은 `OtemPsych/Age-of-War`의 공개 릴리스 에셋을 3DS용으로
축소·변환해 사용합니다. 원작의 음악은 용량과 재생 안정성을 위해 아직 연결하지 않았습니다.
