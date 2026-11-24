# KeyStudy for M-VAVE FM-1

KeyStudy 是一套独立的乐理学习固件。开机进入学习页，默认示例是 **Bm7**。**HOME** 一键进入学习，**SEQ** 一键进入练习，目录、计算器和设置页中也能直接切换。返回快速练习时，会接着上一轮的进度。

## 学习

转动 **SELECT** 选择音名、音程、三和弦、七和弦或音阶。转动 **ALGORITHM** 选择类型，转动 **PRESETS** 更换根音。

屏幕一次显示一组五线谱，默认用高音谱号。和弦按原位叠写，音程和音阶按演奏顺序排列。升降号直接写在音符旁。按 **PLAY** 试听，和弦也可按 **ARP** 逐音试听。谱面和琴键提示使用同一组示范音高，播放到的音符会高亮。

按 **SEL** 打开七张乐理速查卡。转 **SELECT** 浏览读谱口诀、音程判断、三和弦、七和弦、和弦转位、音阶及调号。每张卡有记忆提示和一个按 **PLAY** 即可听的例子。在学习模块按 **GLO** 打开六个计算器。

七和弦包括大七、小七、属七、半减七、减七和小大七。音阶包括大调、自然小调、和声小调及大小调五声音阶。

## 和弦弹奏

开机的 Bm7 是学习示范。按 **SEQ** 进入混合和弦练习。默认每轮从 12 个根音和 12 种三和弦、七和弦中各取不重复的 10 个，逐题显示和弦符号。选择数量较少时，练完一遍才会重新抽取。转动 **SELECT** 也可选单独的三和弦或七和弦题组，以及音名、音程和音阶。

- **Hold**。在琴键上同时按住组成音，固件自动判分。任意转位、八度重复都可通过。
- **Tap**。按 **ARP** 切换到逐音输入。再弹同一个音名可移除，按 **REC** 清空，按 **EDIT** 手动检查。
- **FX** 看提示，**SAVE** 跳题。答对或跳题后按 **EDIT** 进下一题。答对、看提示或跳题会显示答案谱面。按 **PLAY** 可以听示范，计为使用提示。
- 转动 **ALGORITHM** 可切换听辨，按 **PLAY** 听题，转动 **PRESETS** 选类型，再按 **EDIT** 提交。

可直接用 FM-1 自带琴键，也可通过 3.5 mm TRS MIDI IN 或由电脑发送 USB MIDI。成绩保留在当前一轮。

## 乐理活动目录

在练习模块按 **GLO** 打开四组练习，转 **SELECT** 选组、**PRESETS** 选活动，再按 **EDIT** 打开。在学习模块按 **GLO** 打开六个计算器。活动中按 **GLO** 返回本模块的目录。

| 组别 | 活动 | 操作 |
| --- | --- | --- |
| 五线谱识别 | 音名、调号、音程、音阶、和弦 | 看谱答题。转 **PRESETS** 选择，和弦另用 **ALGORITHM** 选根音。**EDIT** 检查。 |
| 五线谱构建 | 音符、调号、音程、音阶、和弦 | **SELECT** 选音符位置，**ALGORITHM** 移动音符，**PRESETS** 改升降号，**SEL** 写入，**REC** 擦除。也可用琴键录入当前音符，再修正写法。调号题用 **PRESETS** 在谱上增减升降号。 |
| 键盘识别 | 音名、反向音名、音程、音阶、和弦 | 根据亮键识别。反向音名题看谱并弹对应琴键。 |
| 听辨 | 听音弹键、听音选音名、音程、音阶、和弦 | 按 **PLAY** 播题，直接弹琴或用 **PRESETS** 选类型。两种单音题先给 C4 作参照。 |
| 计算器 | 临时升降号、音程、音阶、和弦、罗马数字和声、十二音矩阵 | 调参数即时看谱。罗马分析支持大调、自然小调、变化级数、和弦类型和转位。矩阵用琴键输入 12 个不同音级，**REC** 撤回，**FX** 清空。 |

五线谱构建按谱面位置和升降号判分。和弦弹奏按音级判分。键盘识别和听辨使用当前 FM-1 的键盘范围。活动分类及功能说明可参照 [musictheory.net 练习](https://www.musictheory.net/exercises)和[计算器](https://www.musictheory.net/tools)。

## 练习设置

按 **ENV** 打开设置，转 **SELECT** 选择项目，转 **PRESETS** 调整。音程、和弦和音阶的筛选页用 **PRESETS** 浏览类型，按 **SEL** 切换是否纳入。

- 在 **Staff clef** 中选择 **Treble only** 或 **Bass only**。学习示例、练习谱面和计算器使用所选谱号。五线谱单音题和反向音名题会选该谱号附近的可弹音域。此前保存的大谱表选项打开后按高音谱号显示。
- 每轮可设 5 到 30 题。可逐个筛选要练的音程、和弦和音阶，也可以把单音题限定为自然音。
- 听辨音程可依序上行、下行或同时播放。

在 **Tone** 中选择 **Grand Piano** 或 **Pure Tone**。转动 **PRESETS** 切换，当场弹琴即可试听。默认 **Grand Piano** 使用 Yamaha C5 三角钢琴录音。

按 **SAVE** 应用并保存，下次开机仍有效。再按 **ENV** 取消当前修改。改变出题条件会开始新一轮，切换音色可继续当前一轮。

## 试听与安装

- **先试听**。在 [FM-1 Emulator](https://joakimeriksson.github.io/fm1-emu/) 选择 **Open .fwsc…**，打开 [keystudy-0.4.1.fwsc](dist/keystudy-0.4.1.fwsc)。
- **安装到 FM-1**。固件包适用于 M-VAVE FM-1。刷写前备份目前的固件与数据，可使用 [M-Flasher](https://github.com/jvitkauskas/m-flasher)。在已经运行兼容社区固件的 FM-1 上，可用数据线连接并运行下面的更新程序。它会显示识别到的设备，确认后才开始写入。

```sh
python3 base/tools/fm1_install.py dist/keystudy-0.4.1.fwsc
```

更新程序需要 `mido` 和 `python-rtmidi`。首次使用原厂固件或其他引导程序的机器，请按现有固件的安装路径操作。设备固件身份为 `FM-1_54000401`。文件校验值见 [SHA256SUMS](dist/SHA256SUMS)。

## 构建与验证

先安装 Python 3 和 Pillow。真机包另外需要 [JieLi Linux x86‑64 工具链及 AC79 SDK](https://github.com/charlesvestal/fm1-base)，Apple silicon Mac 可用支持 linux/amd64 的 Docker 环境。

```sh
./build.sh --release 0.4.1
./build.sh --host
cc -std=c99 -Wall -Wextra -Werror tests/notation.c -o app/build/host/notation_test
app/build/host/notation_test
for scenario in tests/*.txt; do
  app/build/host/keystudy_host "$scenario" app/build/host || exit
done
```

桌面测试覆盖全部 26 项活动入口、混合和弦出题和判分、谱号切换与保存、五线谱及键盘作答、调号、罗马分析与十二音矩阵。另校验了 1,812 个示范音的记谱音高。

`build.sh` 会取得固定版本的 fm1-base 与 felucca-bsp。可用 `JIELI_TOOLCHAIN`、`AC79_SDK` 和 `PYTHON` 指定安装位置。源代码在 `app/`，生成的固件在 `app/build/`。

KeyStudy 与所用平台代码采用 MPL-2.0。平台来自 [fm1-base](https://github.com/charlesvestal/fm1-base/tree/3c4a57eea659f942b0c80a99949dddafc094a57b) 和 [felucca-bsp](https://github.com/hugelton/felucca-bsp/tree/41a6e86d921f842243bd01a48111832f7e55b4e2)。固件包所含的 JieLi SDK 组件、Barlow 字体及 Bravura 乐谱符号的许可见 [dist/licenses](dist/licenses)。

谱面符号取自 [Bravura](https://github.com/steinbergmedia/bravura/tree/37b194378b710cc40e406ab6c4b07608bb9548ae)。可用 `python3 tools/rasterize_staff.py Bravura.otf app/staff_glyphs.h` 重建符号位图。

钢琴录音的来源、处理和许可见 [Salamander Grand Piano 音源说明](dist/licenses/Salamander-Attribution.md)。音源已打包在固件中。
