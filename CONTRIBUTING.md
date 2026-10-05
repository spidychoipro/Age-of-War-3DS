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
