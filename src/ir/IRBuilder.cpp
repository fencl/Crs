#include "IR.h"

namespace Corrosive {
	void IRBuilder::build_clear(IRBlock* block) {
		block->write_instruction(IRInstruction::Clear);
	}

	void IRBuilder::build_const_i8  (IRBlock* block, int8_t value)   { block->write_instruction(IRInstruction::Const); block->write_const_type(IRConstType::i8);  block->write_value(sizeof(int8_t),   (unsigned char*)&value); }
	void IRBuilder::build_const_i16 (IRBlock* block, int16_t value)  { block->write_instruction(IRInstruction::Const); block->write_const_type(IRConstType::i16); block->write_value(sizeof(int16_t),  (unsigned char*)&value); }
	void IRBuilder::build_const_i32 (IRBlock* block, int32_t value)  { block->write_instruction(IRInstruction::Const); block->write_const_type(IRConstType::i32); block->write_value(sizeof(int32_t),  (unsigned char*)&value); }
	void IRBuilder::build_const_i64 (IRBlock* block, int64_t value)  { block->write_instruction(IRInstruction::Const); block->write_const_type(IRConstType::i64); block->write_value(sizeof(int64_t),  (unsigned char*)&value); }
	void IRBuilder::build_const_u8  (IRBlock* block, uint8_t value)  { block->write_instruction(IRInstruction::Const); block->write_const_type(IRConstType::u8);  block->write_value(sizeof(uint8_t),  (unsigned char*)&value); }
	void IRBuilder::build_const_u16 (IRBlock* block, uint16_t value) { block->write_instruction(IRInstruction::Const); block->write_const_type(IRConstType::u16); block->write_value(sizeof(uint16_t), (unsigned char*)&value); }
	void IRBuilder::build_const_u32 (IRBlock* block, uint32_t value) { block->write_instruction(IRInstruction::Const); block->write_const_type(IRConstType::u32); block->write_value(sizeof(uint32_t), (unsigned char*)&value); }
	void IRBuilder::build_const_u64 (IRBlock* block, uint64_t value) { block->write_instruction(IRInstruction::Const); block->write_const_type(IRConstType::u64); block->write_value(sizeof(uint64_t), (unsigned char*)&value); }
}