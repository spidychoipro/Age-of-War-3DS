# Age of War 3DS

Age of War의 Nintendo 3DS용 2D 디펜스 게임 포트입니다.

## 플레이

- 시작 화면에서 난이도를 고릅니다. 원작 이름 그대로 `Normal`, `Harder`, `Impossible` 중 하나를 터치하거나 방향키로 고른 뒤 `A`를 누릅니다. `HOW TO PLAY`는 원작 설명을 요약한 화면입니다.
- 위 화면은 원작 스프라이트로 그린 전투 화면입니다.
- 아래 화면은 기지 HP, 자원, 시대, 유닛/포탑 생산 UI입니다.
- 아래 화면의 카드 영역을 터치해 유닛을 생산합니다.
- 상단 `UNIT/TURRET` 버튼으로 유닛 생산과 포탑 건설 메뉴를 전환합니다.
- `AGE`는 경험치가 충분할 때 시대를 발전시킵니다.
- `SPECIAL`은 시대별 특수 공격을 사용합니다.
- `SLOTS`는 포탑 슬롯을 확장합니다. 포탑 메뉴의 `SELL`로 슬롯을 선택해 판매할 수 있습니다.
- 배경음악은 타이틀 화면의 `MUSIC ON/OFF`로 켜고 끕니다.
- 위 화면은 터치 입력을 받지 않습니다. 유닛과 기지 체력은 화면의 HP 바로 확인합니다.
- `SELECT`는 일시정지, `A`는 게임 오버 후 재시작, `START`는 메뉴 복귀 확인 창입니다. 확인 창의 `YES`를 터치하거나 `A`를 누르면 타이틀로 돌아갑니다. (`B` 또는 `NO`로 취소)

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

전장 배경과 전체 유닛 애니메이션은 별도 decomp의 PNG를 변환해 사용합니다.
기지·포탑·UI·이펙트 스프라이트와 음악/효과음은 원작 리소스를
`tools/build-assets.ps1`로 변환해 넣었습니다. 원본 디컴파일 결과는 이 저장소에 커밋하지 않고
[`Age-of-War-Flash-Decompiled`](https://github.com/spidychoipro/Age-of-War-Flash-Decompiled)에
따로 뒀습니다. 출처와 권리 상태는 루트의 [`THIRD_PARTY.md`](../THIRD_PARTY.md)를 보세요.

원작 규칙 대조 결과, 회귀 테스트와 남아 있는 차이는
[`FLASH_PARITY.md`](../FLASH_PARITY.md)에 기록합니다. 전체 애니메이션을 다시 만들려면
루트에서 `python tools/build-flash-animations.py <decomp 체크아웃 경로>`를 실행합니다(Pillow 필요).
