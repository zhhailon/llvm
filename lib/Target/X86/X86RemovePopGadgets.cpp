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

using namespace llvm;

#define REMOVE_POP_GADGETS_DESC "X86 Remove Pop Gadgets"
#define REMOVE_POP_GADGETS_NAME "x86-remove-pop-gadgets"

#define DEBUG_TYPE REMOVE_POP_GADGETS_NAME

STATISTIC(NumPopGadgets, "Number of pop gadgets");

namespace {
  class RemovePopGadgetsPass : public MachineFunctionPass {
    // Look over all of the instructions in a basic block.
    void processBasicBlock(MachineFunction &MF, MachineBasicBlock &MBB);

  public:
    static char ID;

    RemovePopGadgetsPass() : MachineFunctionPass(ID) {
      // initializeRemovePopGadgetsPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnMachineFunction(MachineFunction &MF) override;

    MachineFunctionProperties getRequiredProperties() const override {
      return MachineFunctionProperties().set(
          MachineFunctionProperties::Property::NoVRegs);
    }

    StringRef getPassName() const override {
      return REMOVE_POP_GADGETS_DESC;
    }

  private:
    bool isPopOp(MachineInstr &MI);

    const X86Subtarget *STI;
    const TargetInstrInfo *TII;
  };

  char RemovePopGadgetsPass::ID = 0;
}

// INITIALIZE_PASS(RemovePopGadgetsPass, REMOVE_POP_GADGETS_NAME, REMOVE_POP_GADGETS_DESC, false, false)

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
  for (auto &MBB : MF)
    processBasicBlock(MF, MBB);

  dbgs() << "End X86RemovePopGadgets\n";

  return true;
}

void RemovePopGadgetsPass::processBasicBlock(MachineFunction &MF,
                                        MachineBasicBlock &MBB) {
  for (auto I = MBB.begin(); I != MBB.end(); ++I) {
    MachineInstr *MI = &*I;
    if (isPopOp(*MI)) {
      MI->dump();
    }
  }
}

bool RemovePopGadgetsPass::isPopOp(MachineInstr &MI) {
  switch (MI.getOpcode()) {
    case X86::POP64r:
    return true;
  }
  dbgs() << "opcode: " << MI.getOpcode() << " " << MI << "\n";
  return false;
}
