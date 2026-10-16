寄存器（毎个皆一字節）：
  R0  ,  R1  ,  R2  ,  R3  ,  R4  ,  R5  ,  R6  ,  R7
累加器  X坐標 Y坐標 磚塊類型 寬度   高度    値   游戲寄存器（256字節）

可操作地址：
unsigned char CJFX_Global_Map[256][256];
unsigned char gameReg[256];

#define CJFX_REG_Room1 0x00
#define CJFX_REG_Room2 0x01
// 房间序号
#define CJFX_REG_SpawnX 0x02
#define CJFX_REG_SpawnY 0x03
// 出生点
#define CJFX_REG_CameraX 0x04
#define CJFX_REG_CameraY 0x05
// 相机左上点（X与Y）AT LEAST 1 !!!
#define CJFX_REG_CameraLimitW 0x06
#define CJFX_REG_CameraLimitH 0x07
// 相机长宽（WIDTH和HEIGHT）
#define CJFX_REG_JMPRoomU1 0x08
#define CJFX_REG_JMPRoomU2 0x09
// 上
#define CJFX_REG_JMPRoomD1 0x0A
#define CJFX_REG_JMPRoomD2 0x0B
// 下
#define CJFX_REG_JMPRoomL1 0x0C
#define CJFX_REG_JMPRoomL2 0x0D
// 左
#define CJFX_REG_JMPRoomR1 0x0E
#define CJFX_REG_JMPRoomR2 0x0F
// 右
#define CJFX_REG_BG 0x10
// 背景

// 同时更新地图代码。
// 地图代码的值如下，00、01：房间初始值
// 02、03：出生的位置
// 04、05：相机限制 x/y（左上角）
// 06、07：相机限制 w/h（长和宽）
// 08、09，0A、0B，0C、0D，0E、0F：可能跳转的房间号
// 10：使用的背景（0x00~0x0F是特定动态背景）


指令集：

0x00:HALT
終止。

0x01:NOP
空白。

（光標類算子）
0x02:LDX param1
加載至R1。
存儲X坐標。

0x03:LDY param1
加載至R2。
存儲y坐標。

0x04:LDT param1
加載至R3。
存儲磚塊類型。

0x05:LDL param1
存儲寬度（length）。

0x06:LDH param1
存儲高度（height）。

（執行類算子）
0x07:DRAW param1
若為0b00：
絵製x=R1，y=R2，Brick=R3的磚塊。
若為0b01：
批量絵製x=R1，y=R2，Brick=R3，length=R4的磚塊。
若為0b10：
批量絵製x=R1，y=R2，Brick=R3，height=R5的磚塊。
若為0b11：
批量絵製x=R1，y=R2，Brick=R3，length=R4，height=R5的磚塊。

0x08:READ param1 param2
読取x=param1，y=param2的磚塊到R0。

（計算類算子）
0x09:SET param1
賦値R0為Parameter1。

0x0A:ADD param1
将R0与加Parameter1。

0x0B:OR param1
将R0与Parameter1進行或運算。

0x0C:AND param1
将R0与Parameter1進行與運算。

0x0D:XOR param1
将R0与Parameter1進行或運算。

0x0E:LDR param1
将Parameter1指定的寄存器的値加載至R0。

0x0F:RDR param1
将R0的値加載至Parameter1指定的寄存器。

0x10:SUB param1
R0減少Parameter1。

0x11:SHL param1
R0左移Parameter1。

0x12:SHR param1
R0右移Parameter1。

0x13:INC
R0自增1。

0x14:DEC
R0自減1。

（制御類算子）
0x15:JMPF param1
向前無條件跳轉Parameter1字節。

0x16:JMPB param1
向后無條件跳轉Parameter1字節。

0x17:JRF param1 param2
当Parameter1等於R0時向前跳轉Parameter2字節。

0x18:JRB param1 param2
当Parameter1等於R0時向后跳轉Parameter2字節。

0x19:LJMP (unsigned int)param1
絶対跳転到任意地址。

0x1A:RET
返回。

0x1B:RM
ReadMap，読取地図[R2][R1]（R1,R2）的地方到R0。

（寄存器類算子）
0x1C:GETR
将R7對応地址的値写入R6。

0x1D:LDI param1
将Parameter1的値写入R6，
也就是将要写入寄存器的値。

0x1E:SETR param1
将Parameter1的値寫入R7，
也就是将要写入的寄存器地址。

0x1F:APPLY
応用這個1D和1E的値，也就是将R6寫入寄存器中位於R7的地方。


実現代碼：

unsigned char CJFX_Global_Map[256][256];
unsigned char gameReg[256];
#define STACK_MAX 32
// MAXIMUM is 255.
unsigned char CJFX_VMexec(const unsigned char* ProgramName, unsigned short Offset) {
	// Actually a MAP LOADER
	int ProgramPointer = Offset;
	unsigned char REGISTER[8];
	//寄存器（毎个皆一字節）：
	//  R0  , R1(X), R2(Y), R3(T), R4(L), R5(H),  R6  ,  R7
	//累加器  X坐標 Y坐標 磚塊類型 寬度   高度    値   游戲寄存器（256字節）
	unsigned int STACK[STACK_MAX] = {0};
	unsigned char Stack_Pointer = 0;
	// 一个栈指针

	while (1) {
		switch (ProgramName[ProgramPointer]) {
			case 0x01: {
				// 什么也不做。
				ProgramPointer += 1;
				break;//NOP
			}
			case 0x02: {
				REGISTER[1] = ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//LDX param1
			}
			case 0x03: {
				REGISTER[2] = ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//LDY param1
			}
			case 0x04: {
				REGISTER[3] = ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//LDT param1
			}
			case 0x05: {
				REGISTER[4] = ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//LDL param1
			}
			case 0x06: {
				REGISTER[5] = ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//LDH param1
			}
			// 執行類算子
			case 0x07: {
				switch (ProgramName[ProgramPointer + 1]) {
					case 1:
						for (int i = 0; i <= REGISTER[4]; i++) {
							CJFX_Global_Map[REGISTER[2]][REGISTER[1] + i] = REGISTER[3];
						}
						break;//0b01
					case 2:
						for (int i = 0; i <= REGISTER[5]; i++) {
							CJFX_Global_Map[REGISTER[2] + i][REGISTER[1]] = REGISTER[3];
						}
						break;//0b10
					case 3:
						for (int i = 0; i <= REGISTER[5]; i++) {
							for (int j = 0; j <= REGISTER[4]; j++) {
								CJFX_Global_Map[REGISTER[2] + i][REGISTER[1] + j] = REGISTER[3];
							}
						}
						break;//0b11
					default:
						CJFX_Global_Map[REGISTER[2]][REGISTER[1]] = REGISTER[3];
						break;//0b00
				}
				ProgramPointer += 2;
				break;//DRAW param1
			}
			case 0x08: {
				REGISTER[0] = CJFX_Global_Map[ProgramName[ProgramPointer + 2]][ProgramName[ProgramPointer + 1]];
				ProgramPointer += 3;
				break;//READ param1 param2
			}
			//計算類算子
			case 0x09: {
				REGISTER[0] = ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//SET param1
			}
			case 0x0A: {
				REGISTER[0] += ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//ADD param1
			}
			case 0x0B: {
				REGISTER[0] |= ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//OR param1
			}
			case 0x0C: {
				REGISTER[0] &= ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//AND param1
			}
			case 0x0D: {
				REGISTER[0] ^= ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//XOR param1
			}
			case 0x0E: {
				if (ProgramName[ProgramPointer + 1] < 8) {
					REGISTER[0] = REGISTER[ProgramName[ProgramPointer + 1]];
				}
				ProgramPointer += 2;
				break;//LDR param1
			}
			case 0x0F: {
				if (ProgramName[ProgramPointer + 1] < 8) {
					REGISTER[ProgramName[ProgramPointer + 1]] = REGISTER[0];
				}
				ProgramPointer += 2;
				break;//RDR param1
			}
			case 0x10: {
				REGISTER[0] -= ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//SUB param1
			}
			case 0x11: {
				REGISTER[0] <<= (ProgramName[ProgramPointer + 1]) & 7;
				ProgramPointer += 2;
				break;//SHL param1
			}
			case 0x12: {
				REGISTER[0] >>= (ProgramName[ProgramPointer + 1]) & 7;
				ProgramPointer += 2;
				break;//SHR param1
			}
			case 0x13: {
				REGISTER[0] += 1;
				ProgramPointer += 1;
				break;//INC
			}
			case 0x14: {
				REGISTER[0] -= 1;
				ProgramPointer += 1;
				break;//DEC
			}
			//制御類算子
			case 0x15: {
				ProgramPointer += ProgramName[ProgramPointer + 1];
				break;//JMPF param1
			}
			case 0x16: {
				ProgramPointer -= ProgramName[ProgramPointer + 1];
				break;//JMPB param1
			}
			case 0x17: { // JRF param1 param2

				unsigned char compare_value = ProgramName[ProgramPointer + 1];
				unsigned char jump = ProgramName[ProgramPointer + 2];
				if (REGISTER[0] == compare_value) {
					ProgramPointer += jump;
				} else {
					ProgramPointer += 3; // 跳过指令和两个参数
				}
				break;
			}
			case 0x18: { // JRB param1 param2

				unsigned char compare_value = ProgramName[ProgramPointer + 1];
				unsigned char jump = ProgramName[ProgramPointer + 2];
				if (REGISTER[0] == compare_value) {
					ProgramPointer -= jump;
				} else {
					ProgramPointer += 3; // 跳过指令和两个参数
				}
				break;
			}
			case 0x19: {
				if (Stack_Pointer <= STACK_MAX - 1) {
					STACK[Stack_Pointer] = ProgramPointer + 5;
					Stack_Pointer += 1;
					ProgramPointer = *(unsigned int*)(ProgramName + ProgramPointer + 1);
				} else {
					ProgramPointer += 5;
				}
				break;//LJMP
			}
			case 0x1A: {
				if (Stack_Pointer > 0) {
					Stack_Pointer -= 1;
					ProgramPointer = STACK[Stack_Pointer];
					STACK[Stack_Pointer] = 0;
				} else {
					ProgramPointer += 1;
				}
				break;//RET
			}
			case 0x1B: {
				REGISTER[0] = CJFX_Global_Map[REGISTER[2]][REGISTER[1]];
				ProgramPointer += 1;
				break;//RM
			}
			case 0x1C: {
				REGISTER[6] = gameReg[REGISTER[7]];
				ProgramPointer += 1;
				break;//GETR
			}
			case 0x1D: {
				REGISTER[6] = ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//LDI param1
			}
			case 0x1E: {
				REGISTER[7] = ProgramName[ProgramPointer + 1];
				ProgramPointer += 2;
				break;//SETR param1
			}
			case 0x1F: {
				gameReg[REGISTER[7]] = REGISTER[6];
				ProgramPointer += 1;
				break;//APPLY
			}
			default:
				return ProgramName[ProgramPointer];
				break;// 0x00等其他未定义指令返回操作码，0x00是halt，可以返回值：0，即正常。
		}
	}
}