/*
 * LEGO® MINDSTORMS EV3
 *
 * Copyright (C) 2010-2013 The LEGO Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef AM3359_H_
#define AM3359_H_


/*
 * MODE0 - Mux Mode 0
 * MODE1 - Mux Mode 1
 * MODE2 - Mux Mode 2
 * MODE3 - Mux Mode 3
 * MODE4 - Mux Mode 4
 * MODE5 - Mux Mode 5
 * MODE6 - Mux Mode 6
 * MODE7 - Mux Mode 7
 * IDIS - Receiver disabled
 * IEN - Receiver enabled
 * PD - Internal pull-down
 * PU - Internal pull-up
 * OFF - Internal pull disabled
 */

#define MODE0 	0
#define MODE1 	1
#define MODE2 	2
#define MODE3 	3
#define MODE4 	4
#define MODE5 	5
#define MODE6 	6
#define MODE7 	7
#define IDIS 	(0 << 5)
#define IEN 	(1 << 5)
#define PD 	(0 << 3)
#define PU 	(2 << 3)
#define OFF 	(1 << 3)

/*
 * To get the physical address the offset has
 * to be added to AM335X_CTRL_BASE
 */

#define CONTROL_PADCONF_GPMC_AD0                  0x0800
#define CONTROL_PADCONF_GPMC_AD1                  0x0804
#define CONTROL_PADCONF_GPMC_AD2                  0x0808
#define CONTROL_PADCONF_GPMC_AD3                  0x080C
#define CONTROL_PADCONF_GPMC_AD4                  0x0810
#define CONTROL_PADCONF_GPMC_AD5                  0x0814
#define CONTROL_PADCONF_GPMC_AD6                  0x0818
#define CONTROL_PADCONF_GPMC_AD7                  0x081C
#define CONTROL_PADCONF_GPMC_AD8                  0x0820
#define CONTROL_PADCONF_GPMC_AD9                  0x0824
#define CONTROL_PADCONF_GPMC_AD10                 0x0828
#define CONTROL_PADCONF_GPMC_AD11                 0x082C
#define CONTROL_PADCONF_GPMC_AD12                 0x0830
#define CONTROL_PADCONF_GPMC_AD13                 0x0834
#define CONTROL_PADCONF_GPMC_AD14                 0x0838
#define CONTROL_PADCONF_GPMC_AD15                 0x083C
#define CONTROL_PADCONF_GPMC_A0                   0x0840
#define CONTROL_PADCONF_GPMC_A1                   0x0844
#define CONTROL_PADCONF_GPMC_A2                   0x0848
#define CONTROL_PADCONF_GPMC_A3                   0x084C
#define CONTROL_PADCONF_GPMC_A4                   0x0850
#define CONTROL_PADCONF_GPMC_A5                   0x0854
#define CONTROL_PADCONF_GPMC_A6                   0x0858
#define CONTROL_PADCONF_GPMC_A7                   0x085C
#define CONTROL_PADCONF_GPMC_A8                   0x0860
#define CONTROL_PADCONF_GPMC_A9                   0x0864
#define CONTROL_PADCONF_GPMC_A10                  0x0868
#define CONTROL_PADCONF_GPMC_A11                  0x086C
#define CONTROL_PADCONF_GPMC_WAIT0                0x0870
#define CONTROL_PADCONF_GPMC_WPN                  0x0874
#define CONTROL_PADCONF_GPMC_BEN1                 0x0878
#define CONTROL_PADCONF_GPMC_CSN0                 0x087C
#define CONTROL_PADCONF_GPMC_CSN1                 0x0880
#define CONTROL_PADCONF_GPMC_CSN2                 0x0884
#define CONTROL_PADCONF_GPMC_CSN3                 0x0888
#define CONTROL_PADCONF_GPMC_CLK                  0x088C
#define CONTROL_PADCONF_GPMC_ADVN_ALE             0x0890
#define CONTROL_PADCONF_GPMC_OEN_REN              0x0894
#define CONTROL_PADCONF_GPMC_WEN                  0x0898
#define CONTROL_PADCONF_GPMC_BEN0_CLE             0x089C
#define CONTROL_PADCONF_LCD_DATA0                 0x08A0
#define CONTROL_PADCONF_LCD_DATA1                 0x08A4
#define CONTROL_PADCONF_LCD_DATA2                 0x08A8
#define CONTROL_PADCONF_LCD_DATA3                 0x08AC
#define CONTROL_PADCONF_LCD_DATA4                 0x08B0
#define CONTROL_PADCONF_LCD_DATA5                 0x08B4
#define CONTROL_PADCONF_LCD_DATA6                 0x08B8
#define CONTROL_PADCONF_LCD_DATA7                 0x08BC
#define CONTROL_PADCONF_LCD_DATA8                 0x08C0
#define CONTROL_PADCONF_LCD_DATA9                 0x08C4
#define CONTROL_PADCONF_LCD_DATA10                0x08C8
#define CONTROL_PADCONF_LCD_DATA11                0x08CC
#define CONTROL_PADCONF_LCD_DATA12                0x08D0
#define CONTROL_PADCONF_LCD_DATA13                0x08D4
#define CONTROL_PADCONF_LCD_DATA14                0x08D8
#define CONTROL_PADCONF_LCD_DATA15                0x08DC
#define CONTROL_PADCONF_LCD_VSYNC                 0x08E0
#define CONTROL_PADCONF_LCD_HSYNC                 0x08E4
#define CONTROL_PADCONF_LCD_PCLK                  0x08E8
#define CONTROL_PADCONF_LCD_AC_BIAS_EN            0x08EC
#define CONTROL_PADCONF_MMC0_DAT3                 0x08F0
#define CONTROL_PADCONF_MMC0_DAT2                 0x08F4
#define CONTROL_PADCONF_MMC0_DAT1                 0x08F8
#define CONTROL_PADCONF_MMC0_DAT0                 0x08FC
#define CONTROL_PADCONF_MMC0_CLK                  0x0900
#define CONTROL_PADCONF_MMC0_CMD                  0x0904
#define CONTROL_PADCONF_MII1_COL                  0x0908
#define CONTROL_PADCONF_MII1_CRS                  0x090C
#define CONTROL_PADCONF_MII1_RX_ER                0x0910
#define CONTROL_PADCONF_MII1_TX_EN                0x0914
#define CONTROL_PADCONF_MII1_RX_DV                0x0918
#define CONTROL_PADCONF_MII1_TXD3                 0x091C
#define CONTROL_PADCONF_MII1_TXD2                 0x0920
#define CONTROL_PADCONF_MII1_TXD1                 0x0924
#define CONTROL_PADCONF_MII1_TXD0                 0x0928
#define CONTROL_PADCONF_MII1_TX_CLK               0x092C
#define CONTROL_PADCONF_MII1_RX_CLK               0x0930
#define CONTROL_PADCONF_MII1_RXD3                 0x0934
#define CONTROL_PADCONF_MII1_RXD2                 0x0938
#define CONTROL_PADCONF_MII1_RXD1                 0x093C
#define CONTROL_PADCONF_MII1_RXD0                 0x0940
#define CONTROL_PADCONF_RMII1_REF_CLK             0x0944
#define CONTROL_PADCONF_MDIO                      0x0948
#define CONTROL_PADCONF_MDC                       0x094C
#define CONTROL_PADCONF_SPI0_SCLK                 0x0950
#define CONTROL_PADCONF_SPI0_D0                   0x0954
#define CONTROL_PADCONF_SPI0_D1                   0x0958
#define CONTROL_PADCONF_SPI0_CS0                  0x095C
#define CONTROL_PADCONF_SPI0_CS1                  0x0960
#define CONTROL_PADCONF_ECAP0_IN_PWM0_OUT         0x0964
#define CONTROL_PADCONF_UART0_CTSN                0x0968
#define CONTROL_PADCONF_UART0_RTSN                0x096C
#define CONTROL_PADCONF_UART0_RXD                 0x0970
#define CONTROL_PADCONF_UART0_TXD                 0x0974
#define CONTROL_PADCONF_UART1_CTSN                0x0978
#define CONTROL_PADCONF_UART1_RTSN                0x097C
#define CONTROL_PADCONF_UART1_RXD                 0x0980
#define CONTROL_PADCONF_UART1_TXD                 0x0984
#define CONTROL_PADCONF_I2C0_SDA                  0x0988
#define CONTROL_PADCONF_I2C0_SCL                  0x098C
#define CONTROL_PADCONF_MCASP0_ACLKX              0x0990
#define CONTROL_PADCONF_MCASP0_FSX                0x0994
#define CONTROL_PADCONF_MCASP0_AXR0               0x0998
#define CONTROL_PADCONF_MCASP0_AHCLKR             0x099C
#define CONTROL_PADCONF_MCASP0_ACLKR              0x09A0
#define CONTROL_PADCONF_MCASP0_FSR                0x09A4
#define CONTROL_PADCONF_MCASP0_AXR1               0x09A8
#define CONTROL_PADCONF_MCASP0_AHCLKX             0x09AC
#define CONTROL_PADCONF_XDMA_EVENT_INTR0          0x09B0
#define CONTROL_PADCONF_XDMA_EVENT_INTR1          0x09B4
#define CONTROL_PADCONF_WARMRSTN                  0x09B8
#define CONTROL_PADCONF_EXTINTN                   0x09C0
#define CONTROL_PADCONF_TMS                       0x09D0
#define CONTROL_PADCONF_TDI                       0x09D4
#define CONTROL_PADCONF_TDO                       0x09D8
#define CONTROL_PADCONF_TCK                       0x09DC
#define CONTROL_PADCONF_TRSTN                     0x09E0
#define CONTROL_PADCONF_EMU0                      0x09E4
#define CONTROL_PADCONF_EMU1                      0x09E8
#define CONTROL_PADCONF_RTC_PWRONRSTN             0x09F8
#define CONTROL_PADCONF_PMIC_POWER_EN             0x09FC
#define CONTROL_PADCONF_EXT_WAKEUP                0x0A00
#define CONTROL_PADCONF_RTC_KALDO_ENN             0x0A04
#define CONTROL_PADCONF_USB0_DRVVBUS              0x0A1C
#define CONTROL_PADCONF_USB1_DRVVBUS              0x0A34

#define MUX_VAL(OFFSET,VALUE)\
    writel((VALUE), AM335X_CTRL_BASE + (OFFSET));


#define   GPIO_OE                       (0x134 >> 2)    //dir
#define   GPIO_DATAOUT                  (0x13C >> 2)    //out_data
#define   GPIO_SETDATAOUT               (0x194 >> 2)    //set_data
#define   GPIO_CLEARDATAOUT             (0x190 >> 2)    //clr_data
#define   GPIO_DATAIN                   (0x138 >> 2)    //in_data
#define   GPIO_RISINGDETECT             (0x148 >> 2)    //set_rising & clr_rising
#define   GPIO_FALLINGDETECT            (0x14C >> 2)    //set_falling & clr_falling
//#define   GPIO_IRQSTATUS_0            (0x2C  >> 2)    //intstat

#define	  AM33XX_IRQ_UART0		72
#define	  AM33XX_IRQ_UART1		73
#define	  AM33XX_IRQ_UART2		74
#define	  AM33XX_IRQ_UART4		45
#define	  AM33XX_IRQ_UART5		46

enum
{
  GP0_0, GP0_1, GP0_2, GP0_3, GP0_4, GP0_5, GP0_6, GP0_7, GP0_8, GP0_9, GP0_10, GP0_11, GP0_12, GP0_13, GP0_14, GP0_15,
  GP0_16,GP0_17,GP0_18,GP0_19,GP0_20,GP0_21,GP0_22,GP0_23,GP0_24,GP0_25,GP0_26, GP0_27, GP0_28, GP0_29, GP0_30, GP0_31,

  GP1_0, GP1_1, GP1_2, GP1_3, GP1_4, GP1_5, GP1_6, GP1_7, GP1_8, GP1_9, GP1_10, GP1_11, GP1_12, GP1_13, GP1_14, GP1_15,
  GP1_16,GP1_17,GP1_18,GP1_19,GP1_20,GP1_21,GP1_22,GP1_23,GP1_24,GP1_25,GP1_26, GP1_27, GP1_28, GP1_29, GP1_30, GP1_31,

  GP2_0, GP2_1, GP2_2, GP2_3, GP2_4, GP2_5, GP2_6, GP2_7, GP2_8, GP2_9, GP2_10, GP2_11, GP2_12, GP2_13, GP2_14, GP2_15,
  GP2_16,GP2_17,GP2_18,GP2_19,GP2_20,GP2_21,GP2_22,GP2_23,GP2_24,GP2_25,GP2_26, GP2_27, GP2_28, GP2_29, GP2_30, GP2_31,
  
  GP3_0, GP3_1, GP3_2, GP3_3, GP3_4, GP3_5, GP3_6, GP3_7, GP3_8, GP3_9, GP3_10, GP3_11, GP3_12, GP3_13, GP3_14, GP3_15,
  GP3_16,GP3_17,GP3_18,GP3_19,GP3_20,GP3_21,GP3_22,GP3_23,GP3_24,GP3_25,GP3_26, GP3_27, GP3_28, GP3_29, GP3_30, GP3_31
};


typedef   struct
{
  int     Pin;
  u32     Addr;
  u32     Mode;
}
MRM;

MRM       MuxRegMap[] =
{ //  Pin        MUX_ADDR    MUX_DATA



	{GP1_19,	CONTROL_PADCONF_GPMC_A3,	(IEN  | OFF | MODE7)},		//BUFFER_DIS_AIN
	{GP0_31,	CONTROL_PADCONF_GPMC_WPN,	(IEN  | OFF | MODE6)},		//UART4_TXD_AIN
	{GP0_30,	CONTROL_PADCONF_GPMC_WAIT0,	(IEN  | OFF | MODE6)},		//UART4_RXD_AIN
	{GP1_28,	CONTROL_PADCONF_GPMC_BEN1,	(IEN  | OFF | MODE7)},		//DIGIA0_AIN
	{GP1_18,	CONTROL_PADCONF_GPMC_A2,	(IEN  | OFF | MODE7)},		//DIGIA1_AIN
	
	{GP0_4,		CONTROL_PADCONF_SPI0_D1,	(IEN  | OFF | MODE7)},		//BUFFER_DIS_BIN
	{GP0_3,		CONTROL_PADCONF_SPI0_D0,	(IEN  | OFF | MODE1)},		//UART2_TXD_BIN
	{GP0_2,		CONTROL_PADCONF_SPI0_SCLK,	(IEN  | OFF | MODE1)},		//UART2_RXD_BIN
	{GP1_16,	CONTROL_PADCONF_GPMC_A0,	(IEN  | OFF | MODE7)},		//DIGIB0_BIN
	{GP0_5,		CONTROL_PADCONF_SPI0_CS0,	(IEN  | OFF | MODE7)},		//DIGIB1_BIN
	
	{GP3_19,	CONTROL_PADCONF_MCASP0_FSR,	(IEN  | OFF | MODE7)},		//BUFFER_DIS_CIN
	{GP0_15,	CONTROL_PADCONF_UART1_TXD,	(IEN  | OFF | MODE0)},		//UART1_TXD_CIN
	{GP0_14,	CONTROL_PADCONF_UART1_RXD,	(IEN  | OFF | MODE0)},		//UART1_RXD_CIN
	{GP0_12,	CONTROL_PADCONF_UART1_CTSN,	(IEN  | OFF | MODE7)},		//DIGIC0_CIN
	{GP0_13,	CONTROL_PADCONF_UART1_RTSN,	(IEN  | OFF | MODE7)},		//DIGIC1_CIN
	
	{GP3_15,	CONTROL_PADCONF_MCASP0_FSX,	(IEN  | OFF | MODE7)},		//BUFFER_DIS_DIN
	{GP2_14,	CONTROL_PADCONF_LCD_DATA8,	(IEN  | OFF | MODE4)},		//UART5_TXD_DIN
	{GP2_15,	CONTROL_PADCONF_LCD_DATA9,	(IEN  | OFF | MODE4)},		//UART5_RXD_DIN
	{GP3_21,	CONTROL_PADCONF_MCASP0_AHCLKX,	(IEN  | OFF | MODE7)}, 		//DIGID0_DIN
	{GP1_17,	CONTROL_PADCONF_GPMC_A1,	(IEN  | OFF | MODE7)},		//DIGID1_DIN




	{GP1_7, 	CONTROL_PADCONF_GPMC_AD7,	(IEN  | OFF | MODE7)}, 		//DETA
	{GP1_3, 	CONTROL_PADCONF_GPMC_AD3,	(IEN  | OFF | MODE7)},		//DETB
	{GP1_29,	CONTROL_PADCONF_GPMC_CSN0,	(IEN  | OFF | MODE7)},		//DETC
	{GP2_10, 	CONTROL_PADCONF_LCD_DATA4,	(IEN  | OFF | MODE7)},		//DETD




	{GP0_23,	CONTROL_PADCONF_GPMC_AD9,	(IEN  | OFF | MODE4)},		//PWMA
	{GP2_2,		CONTROL_PADCONF_GPMC_ADVN_ALE,	(IEN  | OFF | MODE7)},		//DIR0A
	{GP2_5,		CONTROL_PADCONF_GPMC_BEN0_CLE,	(IEN  | OFF | MODE7)},		//DIR1A
	{GP2_3,		CONTROL_PADCONF_GPMC_OEN_REN,	(IEN  | OFF | MODE7)},		//INTA
	{GP2_4,		CONTROL_PADCONF_GPMC_WEN,	(IEN  | OFF | MODE7)},		//DIRA

	{GP0_22,	CONTROL_PADCONF_GPMC_AD8,	(IEN  | OFF | MODE4)},		//PWMB
	{GP1_13,	CONTROL_PADCONF_GPMC_AD13,	(IEN  | OFF | MODE7)},		//DIR0B
	{GP1_15,	CONTROL_PADCONF_GPMC_AD15,	(IEN  | OFF | MODE7)}, 		//DIR1B
	{GP1_12,	CONTROL_PADCONF_GPMC_AD12,	(IEN  | OFF | MODE7)},		//INTB
	{GP0_26,	CONTROL_PADCONF_GPMC_AD10,	(IEN  | OFF | MODE7)},		//DIRB

	{GP2_17,	CONTROL_PADCONF_LCD_DATA11,	(IEN  | OFF | MODE2)},		//PWMC
	{GP2_12,	CONTROL_PADCONF_LCD_DATA6,	(IEN  | OFF | MODE7)},		//DIR0C
	{GP2_13,	CONTROL_PADCONF_LCD_DATA7,	(IEN  | OFF | MODE7)},		//DIR1C
	{GP0_27,	CONTROL_PADCONF_GPMC_AD11,	(IEN  | OFF | MODE7)},		//DIRC
	{GP1_31,	CONTROL_PADCONF_GPMC_CSN2,	(IEN  | OFF | MODE7)},		//INTC
	
	{GP2_16,	CONTROL_PADCONF_LCD_DATA10,	(IEN  | OFF | MODE2)},		//PWMD
	{GP2_8, 	CONTROL_PADCONF_LCD_DATA2,	(IEN  | OFF | MODE7)},		//DIR0D
	{GP2_7, 	CONTROL_PADCONF_LCD_DATA1,	(IEN  | OFF | MODE7)},		//DIR1D
	{GP2_9, 	CONTROL_PADCONF_LCD_DATA3,	(IEN  | OFF | MODE7)},		//INTD
	{GP2_11, 	CONTROL_PADCONF_LCD_DATA5,	(IEN  | OFF | MODE7)},		//DIRD




	{GP2_23,	CONTROL_PADCONF_LCD_HSYNC,	(IDIS | PD | MODE7)},		//ADC_CLK
	{GP2_24,	CONTROL_PADCONF_LCD_PCLK,	(IDIS | PD | MODE7)},		//ADC_MOSI
	{GP2_22,	CONTROL_PADCONF_LCD_VSYNC,	(IEN  | PD | MODE7)},		//ADC_MISO	
	{GP2_25,	CONTROL_PADCONF_LCD_AC_BIAS_EN,	(IDIS | PU | MODE7)},		//ADC_CS




	{GP0_20,	CONTROL_PADCONF_XDMA_EVENT_INTR1,	(IEN | OFF | MODE7)},	//SOUND_EN
	{GP0_7,		CONTROL_PADCONF_ECAP0_IN_PWM0_OUT,	(IEN | OFF | MODE0)},	//SOUND_PWM




	{GP1_0, 	CONTROL_PADCONF_GPMC_AD0,	(IEN  | OFF | MODE7)},		//ADC_ACK
	{GP1_4, 	CONTROL_PADCONF_GPMC_AD4,	(IEN  | OFF | MODE7)},		//ADCBATEN





	{GP2_6, 	CONTROL_PADCONF_LCD_DATA0,	(IEN  | OFF | MODE7)},		//PEON




//	{GP0_9, 	CONTROL_PADCONF_LCD_DATA13,	(IEN  | OFF | MODE7)}, 		//FB_DC
//	{GP0_8, 	CONTROL_PADCONF_LCD_DATA12,	(IEN  | OFF | MODE7)},		//FB_RST
	{GP1_30,	CONTROL_PADCONF_GPMC_CSN1,	(IEN  | OFF | MODE7)},		//


	{GP1_1, 	CONTROL_PADCONF_GPMC_AD1,	(IEN  | PU | MODE7)},		//
	{GP1_2, 	CONTROL_PADCONF_GPMC_AD2,	(IEN  | OFF | MODE7)},		//
	{GP1_5, 	CONTROL_PADCONF_GPMC_AD5,	(IEN  | PU | MODE7)},		//
	{GP1_6, 	CONTROL_PADCONF_GPMC_AD6,	(IEN  | OFF | MODE7)}, 		//
	{GP1_14,	CONTROL_PADCONF_GPMC_AD14,	(IEN  | PU | MODE7)}, 		//
	{GP2_1,		CONTROL_PADCONF_GPMC_CLK,	(IEN  | PU | MODE7)}, 		//
	


    	{-1 }
};


typedef struct
{
	int Pin;      // GPIO pin number
	volatile u32 *pGpio;   // GPIO bank base address
	u32 Mask;     // GPIO pin mask
}
INPIN;


#else

extern    MRM MuxRegMap[];

#endif /* AM3359_H_ */
