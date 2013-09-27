#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/NewTester.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/NewTester.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED="../Libs/Microchip/MDD File System/FSIO.c" "../Libs/Microchip/MDD File System/SD-SPI.c" ../Traps.c main.c ../Libs/Uart2/Uart2.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/200304081/FSIO.o ${OBJECTDIR}/_ext/200304081/SD-SPI.o ${OBJECTDIR}/_ext/1472/Traps.o ${OBJECTDIR}/main.o ${OBJECTDIR}/_ext/972947182/Uart2.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/200304081/FSIO.o.d ${OBJECTDIR}/_ext/200304081/SD-SPI.o.d ${OBJECTDIR}/_ext/1472/Traps.o.d ${OBJECTDIR}/main.o.d ${OBJECTDIR}/_ext/972947182/Uart2.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/200304081/FSIO.o ${OBJECTDIR}/_ext/200304081/SD-SPI.o ${OBJECTDIR}/_ext/1472/Traps.o ${OBJECTDIR}/main.o ${OBJECTDIR}/_ext/972947182/Uart2.o

# Source Files
SOURCEFILES=../Libs/Microchip/MDD File System/FSIO.c ../Libs/Microchip/MDD File System/SD-SPI.c ../Traps.c main.c ../Libs/Uart2/Uart2.c


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/NewTester.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=33EP256MC502
MP_LINKER_FILE_OPTION=,--script=p33EP256MC502.gld
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/200304081/FSIO.o: ../Libs/Microchip/MDD\ File\ System/FSIO.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/200304081 
	@${RM} ${OBJECTDIR}/_ext/200304081/FSIO.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  "../Libs/Microchip/MDD File System/FSIO.c"  -o ${OBJECTDIR}/_ext/200304081/FSIO.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/200304081/FSIO.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1  -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/_ext/200304081/FSIO.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/200304081/SD-SPI.o: ../Libs/Microchip/MDD\ File\ System/SD-SPI.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/200304081 
	@${RM} ${OBJECTDIR}/_ext/200304081/SD-SPI.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  "../Libs/Microchip/MDD File System/SD-SPI.c"  -o ${OBJECTDIR}/_ext/200304081/SD-SPI.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/200304081/SD-SPI.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1  -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/_ext/200304081/SD-SPI.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/Traps.o: ../Traps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/Traps.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../Traps.c  -o ${OBJECTDIR}/_ext/1472/Traps.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/Traps.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1  -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/Traps.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/main.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/main.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1  -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/972947182/Uart2.o: ../Libs/Uart2/Uart2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/972947182 
	@${RM} ${OBJECTDIR}/_ext/972947182/Uart2.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../Libs/Uart2/Uart2.c  -o ${OBJECTDIR}/_ext/972947182/Uart2.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/972947182/Uart2.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1  -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/_ext/972947182/Uart2.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
else
${OBJECTDIR}/_ext/200304081/FSIO.o: ../Libs/Microchip/MDD\ File\ System/FSIO.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/200304081 
	@${RM} ${OBJECTDIR}/_ext/200304081/FSIO.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  "../Libs/Microchip/MDD File System/FSIO.c"  -o ${OBJECTDIR}/_ext/200304081/FSIO.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/200304081/FSIO.o.d"      -g -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/_ext/200304081/FSIO.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/200304081/SD-SPI.o: ../Libs/Microchip/MDD\ File\ System/SD-SPI.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/200304081 
	@${RM} ${OBJECTDIR}/_ext/200304081/SD-SPI.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  "../Libs/Microchip/MDD File System/SD-SPI.c"  -o ${OBJECTDIR}/_ext/200304081/SD-SPI.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/200304081/SD-SPI.o.d"      -g -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/_ext/200304081/SD-SPI.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1472/Traps.o: ../Traps.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/Traps.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../Traps.c  -o ${OBJECTDIR}/_ext/1472/Traps.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/Traps.o.d"      -g -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/Traps.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/main.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/main.o.d"      -g -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/972947182/Uart2.o: ../Libs/Uart2/Uart2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/972947182 
	@${RM} ${OBJECTDIR}/_ext/972947182/Uart2.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ../Libs/Uart2/Uart2.c  -o ${OBJECTDIR}/_ext/972947182/Uart2.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/972947182/Uart2.o.d"      -g -omf=elf -mlarge-data -O0 -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/NewSDWrite" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Timer2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart1" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Uart2" -I"/home/asl/Documents/jdharkin/SLUGS-Logger" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/Microchip/Include" -I"/home/asl/Documents/jdharkin/SLUGS-Logger/Libs/DEE" -I"../Libs/Microchip/Include" -I"../" -I"../Libs/Uart2" -msmart-io=1 -Wall -msfr-warn=off
	@${FIXDEPS} "${OBJECTDIR}/_ext/972947182/Uart2.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/NewTester.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk  ../Libs/DEE/Flash\ Operations\ 33E_24E.s ../stack.S  
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/NewTester.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}    "../Libs/DEE/Flash Operations 33E_24E.s" ../stack.S  -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1  -omf=elf -Wl,--defsym=__MPLAB_BUILD=1,--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_PK3=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem$(MP_EXTRA_LD_POST) 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/NewTester.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk  ../Libs/DEE/Flash\ Operations\ 33E_24E.s ../stack.S 
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/NewTester.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}    "../Libs/DEE/Flash Operations 33E_24E.s" ../stack.S  -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -Wl,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem$(MP_EXTRA_LD_POST) 
	${MP_CC_DIR}/xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/NewTester.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf 
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell "${PATH_TO_IDE_BIN}"mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
