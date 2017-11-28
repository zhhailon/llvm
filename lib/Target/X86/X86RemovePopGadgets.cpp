//===-------- X86RemovePops.cpp - remove pops -----------===//

#include <algorithm>

#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include <cassert>

using namespace llvm;

#define REMOVE_POP_GADGETS_DESC "X86 Remove Pop Gadgets"
#define REMOVE_POP_GADGETS_NAME "x86-remove-pop-gadgets"

#define DEBUG_TYPE REMOVE_POP_GADGETS_NAME

STATISTIC(NumPops, "Number of POPs");

namespace {
  class RemovePopGadgetsPass : public MachineFunctionPass {
  public:
    static char ID;

    RemovePopGadgetsPass() : MachineFunctionPass(ID) {
      // initializeRemovePopGadgetsPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnMachineFunction(MachineFunction &MF) override;

    // MachineFunctionProperties getRequiredProperties() const override {
    //   return MachineFunctionProperties().set(
    //       MachineFunctionProperties::Property::NoVRegs);
    // }

    StringRef getPassName() const override {
      return REMOVE_POP_GADGETS_DESC;
    }

  private:
    // Look over all of the instructions in a basic block.
    void processBasicBlock(MachineFunction &MF, MachineBasicBlock &MBB);

    bool isPopOp(MachineInstr &MI);
    bool isPushOp(MachineInstr &MI);
    bool isPopOperandRAX(MachineInstr &MI);
    bool isPushOperandRAX(MachineInstr &MI);
    bool isPopOperandRBX(MachineInstr &MI);
    bool isPushOperandRBX(MachineInstr &MI);

    const X86Subtarget *STI;
    const TargetInstrInfo *TII;
  };

  char RemovePopGadgetsPass::ID = 0;
}

INITIALIZE_PASS(RemovePopGadgetsPass, REMOVE_POP_GADGETS_NAME, REMOVE_POP_GADGETS_DESC, false, true)

FunctionPass *llvm::createX86RemovePopGadgets() {
  return new RemovePopGadgetsPass();
}

/// runOnMachineFunction - Loop over all of the basic blocks, inserting
/// NOOP instructions before early exits.
bool RemovePopGadgetsPass::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(*MF.getFunction()))
    return false;

  STI = &MF.getSubtarget<X86Subtarget>();
  TII = STI->getInstrInfo();

  dbgs() << "Start X86RemovePopGadgets\n";

  // Process all basic blocks.
  for (auto &MBB : MF) {
    processBasicBlock(MF, MBB);
  }

  dbgs() << "End X86RemovePopGadgets\n";

  return true;
}

void RemovePopGadgetsPass::processBasicBlock(MachineFunction &MF,
                                        MachineBasicBlock &MBB) {
  for (auto I = MBB.begin(); I != MBB.end(); ++I) {
    MachineInstr *MI = &*I;

    if (isPopOp(*MI)) {
      dbgs() << "opcode: " << MI->getOpcode() << " " << *MI << "\n";
      // llvm/lib/Target/X86/X86ISelLowering.cpp#25270
      BuildMI(MBB, MI, MI->getDebugLoc(), TII->get(X86::CALL64pcrel32))
        .addExternalSymbol("heap_stack_pop");
    } else if (isPushOp(*MI)) {
      dbgs() << "opcode: " << MI->getOpcode() << " " << *MI << "\n";
      BuildMI(MBB, MI, MI->getDebugLoc(), TII->get(X86::CALL64pcrel32))
        .addExternalSymbol("heap_stack_push");
    }
  }
  MBB.dump();
}

bool RemovePopGadgetsPass::isPopOperandRAX(MachineInstr &MI) {
  assert(isPopOp(MI) && "Not POP instruction");
  const MachineOperand operand = MI.getOperand(0);
  return operand.isReg() && operand.getReg() == X86::RAX;
}

bool RemovePopGadgetsPass::isPushOperandRAX(MachineInstr &MI) {
  assert(isPushOp(MI) && "Not PUSH instruction");
  const MachineOperand operand = MI.getOperand(0);
  return operand.isReg() && operand.getReg() == X86::RAX;
}

bool RemovePopGadgetsPass::isPopOperandRBX(MachineInstr &MI) {
  assert(isPopOp(MI) && "Not POP instruction");
  const MachineOperand operand = MI.getOperand(0);
  return operand.isReg() && operand.getReg() == X86::RBX;
}

bool RemovePopGadgetsPass::isPushOperandRBX(MachineInstr &MI) {
  assert(isPushOp(MI) && "Not PUSH instruction");
  const MachineOperand operand = MI.getOperand(0);
  return operand.isReg() && operand.getReg() == X86::RBX;
}

bool RemovePopGadgetsPass::isPushOp(MachineInstr &MI) {
  switch (MI.getOpcode()) {
    case X86::PUSH16i8:
    case X86::PUSH16r:
    case X86::PUSH16rmm:
    case X86::PUSH16rmr:
    case X86::PUSH32i8:
    case X86::PUSH32r:
    case X86::PUSH32rmm:
    case X86::PUSH32rmr:
    case X86::PUSH64i32:
    case X86::PUSH64i8:
    case X86::PUSH64r:
    case X86::PUSH64rmm:
    case X86::PUSH64rmr:
    return true;
  }
  return false;
}

bool RemovePopGadgetsPass::isPopOp(MachineInstr &MI) {
  switch (MI.getOpcode()) {
    case X86::POP16r:
    case X86::POP16rmm:
    case X86::POP16rmr:
    case X86::POP32r:
    case X86::POP32rmm:
    case X86::POP32rmr:
    case X86::POP64r:
    case X86::POP64rmm:
    case X86::POP64rmr:
    ++NumPops;
    return true;
  }
  return false;
}
