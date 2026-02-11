unsigned char CJFX_Global_Map[256][256];
unsigned char gameReg[256];
unsigned char CJFX_VMexec(const unsigned char* ProgramName,unsigned short Offset){
	int ProgramPointer=Offset;
	unsigned char REGISTER[8];
	//寄存器（毎个皆一字節）：
	//  R0  ,  R1  ,  R2  ,  R3  ,  R4  ,  R5  ,  R6  ,  R7
	//累加器  X坐標 Y坐標 磚塊類型 寬度   高度    値   游戲寄存器（256字節）
	while(1){
		switch (ProgramName[ProgramPointer]) {
			case 0x01:{
				// 什麼也不做。
				ProgramPointer+=1;
				break;
			}
			case 0x02:{
				REGISTER[1]=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//LDX param1
			}
			case 0x03:{
				REGISTER[2]=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//LDY param1
			}
			case 0x04:{
				REGISTER[3]=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//LDT param1
			}
			case 0x05:{
				REGISTER[4]=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//LDL param1
			}
			case 0x06:{
				REGISTER[5]=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//LDH param1
			}
			// 執行類算子
			case 0x07:{
				switch (ProgramName[ProgramPointer+1]) {
					case 1:
						for(int i=0;i<=REGISTER[4];i++){
							CJFX_Global_Map[REGISTER[2]][REGISTER[1]+i]=REGISTER[3];
						}
						break;//0b01
					case 2:
						for(int i=0;i<=REGISTER[5];i++){
							CJFX_Global_Map[REGISTER[2]+i][REGISTER[1]]=REGISTER[3];
						}
						break;//0b10
					case 3:
						for(int i=0;i<=REGISTER[5];i++){
							for(int j=0;j<=REGISTER[4];j++){
								CJFX_Global_Map[REGISTER[2]+i][REGISTER[1]+j]=REGISTER[3];
							}
						}					
						break;//0b11
					default:
						CJFX_Global_Map[REGISTER[2]][REGISTER[1]]=REGISTER[3];
						break;//0b00
				}
				ProgramPointer+=2;
				break;//DRAW param1
			}
			case 0x08:{
				REGISTER[0]=CJFX_Global_Map[ProgramName[ProgramPointer+2]][ProgramName[ProgramPointer+1]];
				ProgramPointer+=3;
				break;//READ param1 param2
			}
				//計算類算子
			case 0x09:{
				REGISTER[0]=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//SET param1
			}
			case 0x0A:{
				REGISTER[0]+=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//ADD param1
			}
			case 0x0B:{
				REGISTER[0]|=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//OR param1
			}
			case 0x0C:{
				REGISTER[0]&=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//AND param1
			}
			case 0x0D:{
				REGISTER[0]^=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//XOR param1
			}
			case 0x0E:{
				if (ProgramName[ProgramPointer+1] < 8) {
				    REGISTER[0] = REGISTER[ProgramName[ProgramPointer+1]];
				}
				ProgramPointer+=2;
				break;//LDR param1
			}
			case 0x0F:{
				if (ProgramName[ProgramPointer+1] < 8) {
				    REGISTER[ProgramName[ProgramPointer+1]] = REGISTER[0];
				}
				ProgramPointer+=2;
				break;//RDR param1
			}
			case 0x10:{
				REGISTER[0]-=ProgramName[ProgramPointer+1];
				ProgramPointer+=2;
				break;//SUB param1
			}
			case 0x11:{
				REGISTER[0]<<=(ProgramName[ProgramPointer+1])&7;
				ProgramPointer+=2;
				break;//SHL param1
			}
			case 0x12:{
				REGISTER[0]>>=(ProgramName[ProgramPointer+1])&7;
				ProgramPointer+=2;
				break;//SHR param1
			}
			case 0x13:{
				REGISTER[0]+=1;
				ProgramPointer+=1;
				break;//INC
			}
			case 0x14:{
				REGISTER[0]-=1;
				ProgramPointer+=1;
				break;//DEC
			}
				//制御類算子
			case 0x15:{
				ProgramPointer+=ProgramName[ProgramPointer+1];
				break;//JMPF param1
			}
			case 0x16:{
				ProgramPointer-=ProgramName[ProgramPointer+1];
				break;//JMPB param1
			}
 			case 0x17:{ // JRF param1 param2
            
                unsigned char compare_value = ProgramName[ProgramPointer + 1];
                unsigned char jump = ProgramName[ProgramPointer + 2];
                if(REGISTER[0] == compare_value) {
                    ProgramPointer += jump;
                } else {
                    ProgramPointer += 3; // 跳過指令和両个参数
                }
                break;
            }
            case 0x18:{ // JRB param1 param2
            
                unsigned char compare_value = ProgramName[ProgramPointer + 1];
                unsigned char jump = ProgramName[ProgramPointer + 2];
                if(REGISTER[0] == compare_value) {
                    ProgramPointer -= jump;
                } else {
                    ProgramPointer += 3; // 跳過指令和両个参数
                }
                break;
            }   
			case 0x19://Unused
			case 0x1A://Unused
			case 0x1B:{
				ProgramPointer+=1;
				break;//Unused
			}
			case 0x1C:{
				REGISTER[6]=gameReg[REGISTER[7]];
				ProgramPointer+=1;
				break;//GETR
			}
			case 0x1D:{
				REGISTER[6]=ProgramName[ProgramPointer + 1];
				ProgramPointer+=2;
				break;//LDI param1
			}
			case 0x1E:{
				REGISTER[7]=ProgramName[ProgramPointer + 1];
				ProgramPointer+=2;
				break;//SETR param1
			}
			case 0x1F:{
				gameReg[REGISTER[7]]=REGISTER[6];
				ProgramPointer+=1;
				break;//APPLY
			}
			default:
				return ProgramName[ProgramPointer];
				break;// 0x00等其他未定義指令返回操作碼，0x00是halt，可以返回値：0，即正常。
		}
	}
}
