#include "IL.hpp"
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace Crs {

	bool static_memory_sort(const std::tuple<ILSize,std::unique_ptr<unsigned char[]>,std::uint32_t, std::size_t>& l, const std::tuple<ILSize,std::unique_ptr<unsigned char[]>,std::uint32_t, std::size_t>& r) {
		return std::get<3>(l) < std::get<3>(r);
	}

	void ILStructTable::clean_prepass(ILModule& mod, std::unordered_set<std::size_t>& used_tables, std::unordered_set<std::size_t>& used_arrays) {
		for (auto&& elem: elements) {
			if (elem.type == ILSizeType::table) {
				auto ins = used_tables.insert(elem.value);
				if (ins.second) {
					mod.structure_tables[elem.value].clean_prepass(mod,used_tables,used_arrays);
				}
			} else if (elem.type == ILSizeType::array) {
				auto ins = used_arrays.insert(elem.value);
				if (ins.second) {
					mod.array_tables[elem.value].clean_prepass(mod,used_tables,used_arrays);
				}
			}
		}
	}

	void ILArrayTable::clean_prepass(ILModule& mod, std::unordered_set<std::size_t>& used_tables, std::unordered_set<std::size_t>& used_arrays) {		
		if (element.type == ILSizeType::table) {
			auto ins = used_tables.insert(element.value);
			if (ins.second) {
				mod.structure_tables[element.value].clean_prepass(mod,used_tables,used_arrays);
			}
		} else if (element.type == ILSizeType::array) {
			auto ins = used_arrays.insert(element.value);
			if (ins.second) {
				mod.array_tables[element.value].clean_prepass(mod,used_tables,used_arrays);
			}
		}
	}

	void ILStructTable::clean_pass(ILModule& mod, std::unordered_map<std::size_t, std::size_t>& map_tables, std::unordered_map<std::size_t, std::size_t>& map_arrays){
		for (auto&& elem: elements) {
			if (elem.type == ILSizeType::table) {
				elem.value = (tableid_t)map_tables[elem.value];
			}else if (elem.type == ILSizeType::array) {
				elem.value = (tableid_t)map_arrays[elem.value];
			}
		}
	}

	void ILArrayTable::clean_pass(ILModule& mod, std::unordered_map<std::size_t, std::size_t>& map_tables, std::unordered_map<std::size_t, std::size_t>& map_arrays){
		if (element.type == ILSizeType::table) {
			element.value = (tableid_t)map_tables[element.value];
		}else if (element.type == ILSizeType::array) {
			element.value = (tableid_t)map_arrays[element.value];
		}
	}

	void ILModule::strip_unused_content() {
		std::unordered_set<std::size_t> used_functions;
		std::unordered_set<std::size_t> used_constants;
		std::unordered_set<std::size_t> used_statics;
		std::unordered_set<std::size_t> used_vtables;
		std::unordered_set<std::size_t> used_decls;
		std::unordered_set<std::size_t> used_tables;
		std::unordered_set<std::size_t> used_arrays;


		std::vector<ILFunction*> remaining_build;
		for (auto&& f : exported_functions) {
			used_functions.insert(f->id);
			remaining_build.push_back(f);
		}

		for (auto&& f : remaining_build) {
			if (auto bf = dynamic_cast<ILBytecodeFunction*>(f)) {
				bf->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
			}
		}

		for (auto&& tid : used_tables) {
			structure_tables[tid].clean_prepass(*this, used_tables, used_arrays);
		}
		
		for (auto&& tid : used_arrays) {
			array_tables[tid].clean_prepass(*this, used_tables, used_arrays);
		}

		std::unordered_map<std::size_t, std::size_t> map_functions;
		std::unordered_map<std::size_t, std::size_t> map_constants;
		std::unordered_map<std::size_t, std::size_t> map_statics;
		std::unordered_map<std::size_t, std::size_t> map_vtables;
		std::unordered_map<std::size_t, std::size_t> map_decls;
		std::unordered_map<std::size_t, std::size_t> map_tables;
		std::unordered_map<std::size_t, std::size_t> map_arrays;
		external_functions.clear();

		std::vector<std::unique_ptr<ILFunction>> new_functions; 
		for (auto&& used_func : used_functions) {
			map_functions[used_func] = new_functions.size();
			new_functions.push_back(std::move(functions[used_func]));
		}
		functions = std::move(new_functions);

		std::vector<std::pair<ILSize, std::unique_ptr<unsigned char[]>>> new_constants;
		for (auto&& used_constant : used_constants) {
			map_constants[used_constant] = new_constants.size();
			new_constants.push_back(std::move(constant_memory[used_constant]));
		}
		constant_memory = std::move(new_constants);

		std::vector<std::tuple<ILSize, std::unique_ptr<unsigned char[]>, std::uint32_t, std::size_t>> new_statics;
		for (auto&& used_static : used_statics) {
			map_statics[used_static] = new_statics.size();
			new_statics.push_back(std::move(static_memory[used_static]));
			std::uint32_t& funid = std::get<2>(new_statics.back());
			if (funid != UINT32_MAX)
				funid = (std::uint32_t)map_functions[funid];
		}
		static_memory = std::move(new_statics);
		std::sort(static_memory.begin(), static_memory.end(), static_memory_sort);
		for (std::size_t i=0; i< static_memory.size(); ++i) {
			std::get<3>(static_memory[i]) = i;	
		}


		std::vector<std::pair<std::uint32_t,std::unique_ptr<void* []>>> new_vtables;
		for (auto&& used_vtable : used_vtables) {
			map_vtables[used_vtable] = new_vtables.size();
			new_vtables.push_back(std::move(vtable_data[used_vtable]));
		}
		vtable_data = std::move(new_vtables);


		std::vector<std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>> new_decls;
		for (auto&& used_decl : used_decls) {
			map_decls[used_decl] = new_decls.size();
			new_decls.push_back(std::move(function_decl[used_decl]));
		}
		function_decl = std::move(new_decls);


		std::vector<ILStructTable> new_tables;
		for (auto&& used_table : used_tables) {
			map_tables[used_table] = new_tables.size();
			new_tables.push_back(std::move(structure_tables[used_table]));
		}
		structure_tables = std::move(new_tables);


		std::vector<ILArrayTable> new_arrays;
		for (auto&& used_array : used_arrays) {
			map_arrays[used_array] = new_arrays.size();
			new_arrays.push_back(std::move(array_tables[used_array]));
		}
		array_tables = std::move(new_arrays);


		for (auto&& table : structure_tables) {
			for (auto&& elem : table.elements) {
				if (elem.type == ILSizeType::table) {
					elem.value = (tableid_t)map_tables[elem.value];
				}
				else if (elem.type == ILSizeType::array) {
					elem.value = (tableid_t)map_arrays[elem.value];
				}
			}
		}

		for (auto&& table : array_tables) {
			if (table.element.type == ILSizeType::table) {
				table.element.value = (tableid_t)map_tables[table.element.value];
			}
			else if (table.element.type == ILSizeType::array) {
				table.element.value = (tableid_t)map_arrays[table.element.value];
			}
		}

		for (auto&& f : map_functions) {
			auto fun = functions[f.second].get();
			fun->decl_id = (std::uint32_t)map_decls[fun->decl_id];
			fun->id = (std::uint32_t)map_functions[fun->id];

			if (auto bf = dynamic_cast<ILBytecodeFunction*>(fun)) {
				bf->clean_pass(map_functions, map_constants, map_statics, map_vtables, map_decls, map_tables, map_arrays);
			}else {
				ILNativeFunction* nfun = (ILNativeFunction*)fun;
				external_functions[nfun->name] = fun->id;
			}
		}

		for (auto&& t : structure_tables) {
			t.clean_pass(*this, map_tables, map_arrays);
		}

		for (auto&& t : array_tables) {
			t.clean_pass(*this, map_tables, map_arrays);
		}

		//rebuild_pointer_table();
	}

	void ILBytecodeFunction::clean_prepass(std::unordered_set<std::size_t>& used_functions,
		std::unordered_set<std::size_t>& used_constants,
		std::unordered_set<std::size_t>& used_statics,
		std::unordered_set<std::size_t>& used_vtables,
		std::unordered_set<std::size_t>& used_decls,
		std::unordered_set<std::size_t>& used_tables,
		std::unordered_set<std::size_t>& used_arrays) {
		
		used_decls.insert(decl_id);
		
		local_stack_lifetime.clean_prepass(used_tables, used_arrays);

		for (auto&& block : blocks) {

			auto it = block->data_pool.begin();

			while (it != block->data_pool.end()) {

				auto inst = ILBlock::read_data<ILInstruction>(it);

				switch (inst) {
					case ILInstruction::ret: {
						ILBlock::read_data<ILDataType>(it);
					} break;
					case ILInstruction::call8: {
						used_decls.insert(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::call16: {
						used_decls.insert(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::call32: {
						used_decls.insert(ILBlock::read_data<std::uint32_t>(it));
					} break;
					case ILInstruction::memcpy: {
						auto s = ILBlock::read_data<ILSize>(it);
						if (s.type == ILSizeType::table) {
							used_tables.insert(s.value);
						}
						else if (s.type == ILSizeType::array) {
							used_arrays.insert(s.value);
						}
					} break;
					case ILInstruction::memcpy2: {
						auto s = ILBlock::read_data<ILSize>(it);
						if (s.type == ILSizeType::table) {
							used_tables.insert(s.value);
						}
						else if (s.type == ILSizeType::array) {
							used_arrays.insert(s.value);
						}
					} break;
					case ILInstruction::memcmp: {
						auto s = ILBlock::read_data<ILSize>(it);
						if (s.type == ILSizeType::table) {
							used_tables.insert(s.value);
						}
						else if (s.type == ILSizeType::array) {
							used_arrays.insert(s.value);
						}
					} break;
					case ILInstruction::memcmp2: {
						auto s = ILBlock::read_data<ILSize>(it);
						if (s.type == ILSizeType::table) {
							used_tables.insert(s.value);
						}
						else if (s.type == ILSizeType::array) {
							used_arrays.insert(s.value);
						}
					} break;
					case ILInstruction::fnptr8: {
						auto r = used_functions.insert(ILBlock::read_data<std::uint8_t>(it));
						if (r.second) {
							if (auto bf = dynamic_cast<ILBytecodeFunction*>(parent->functions[*r.first].get())) {
								bf->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
							}
						}
					} break;
					case ILInstruction::fnptr16: {
						auto r = used_functions.insert(ILBlock::read_data<std::uint16_t>(it));
						if (r.second) {
							if (auto bf = dynamic_cast<ILBytecodeFunction*>(parent->functions[*r.first].get())) {
								bf->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
							}
						}
					} break;
					case ILInstruction::fnptr32: {
						auto r = used_functions.insert(ILBlock::read_data<std::uint32_t>(it));
						if (r.second) {
							if (auto bf = dynamic_cast<ILBytecodeFunction*>(parent->functions[*r.first].get())) {
								bf->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
							}
						}
					} break;

					case ILInstruction::fncall8: {
						auto r = used_functions.insert(ILBlock::read_data<std::uint8_t>(it));
						if (r.second) {
							if (auto bf = dynamic_cast<ILBytecodeFunction*>(parent->functions[*r.first].get())) {
								bf->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
							}
						}
					} break;
					case ILInstruction::fncall16: {
						auto r = used_functions.insert(ILBlock::read_data<std::uint16_t>(it));
						if (r.second) {
							if (auto bf = dynamic_cast<ILBytecodeFunction*>(parent->functions[*r.first].get())) {
								bf->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
							}
						}
					} break;
					case ILInstruction::fncall32: {
						auto r = used_functions.insert(ILBlock::read_data<std::uint32_t>(it));
						if (r.second) {
							if (auto bf = dynamic_cast<ILBytecodeFunction*>(parent->functions[*r.first].get())) {
								bf->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
							}
						}
					} break;
					case ILInstruction::vtable: {
						auto vtableid = ILBlock::read_data<std::uint32_t>(it);
						used_vtables.insert(vtableid);
						auto& table = parent->vtable_data[vtableid];
						ILBytecodeFunction** table_data = (ILBytecodeFunction**)table.second.get();
						std::uint32_t size = table.first;

						for (std::uint32_t i = 0; i < size; ++i) {
							ILBytecodeFunction* fun = table_data[i];
							used_functions.insert(fun->id);
							fun->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
						}
						
					} break;
					case ILInstruction::duplicate: {
						ILBlock::read_data<ILDataType>(it);
					} break;
					case ILInstruction::clone: {
						ILBlock::read_data<ILDataType>(it);
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::swap: {
						ILBlock::read_data<ILDataType>(it);
					} break;
					case ILInstruction::swap2: {
						ILBlock::read_data<ILDataTypePair>(it);
					} break;
					case ILInstruction::insintric: {
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::rmemcmp: {
						ILBlock::read_data<ILDataType>(it);
					} break;
					case ILInstruction::rmemcmp2: {
						ILBlock::read_data<ILDataType>(it);
					} break;
					case ILInstruction::sub:
					case ILInstruction::div:
					case ILInstruction::rem:
					case ILInstruction::mul:
					case ILInstruction::add:
					case ILInstruction::bit_and:
					case ILInstruction::bit_or:
					case ILInstruction::bit_xor:
					case ILInstruction::eq:
					case ILInstruction::ne:
					case ILInstruction::gt:
					case ILInstruction::lt:
					case ILInstruction::ge:
					case ILInstruction::le: {
						ILBlock::read_data<ILDataTypePair>(it);
					} break;

					case ILInstruction::cast:
					case ILInstruction::bitcast: {
						ILBlock::read_data<ILDataTypePair>(it);
					} break;

					case ILInstruction::store:
					case ILInstruction::store2:
					case ILInstruction::yield: 
					case ILInstruction::accept:
					case ILInstruction::discard: {
						ILBlock::read_data<ILDataType>(it);
					} break;


					case ILInstruction::jmp8: {
						ILBlock::read_data<std::uint8_t>(it);
					}break;
					case ILInstruction::jmp16: {
						ILBlock::read_data<std::uint16_t>(it);
					}break;
					case ILInstruction::jmp32: {
						ILBlock::read_data<std::uint32_t>(it);
					}break;

					case ILInstruction::offset32: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;
					case ILInstruction::offset16: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;
					case ILInstruction::offset8: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;
					case ILInstruction::aoffset8: {
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::aoffset16: {
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::aoffset32: {
						ILBlock::read_data<std::uint32_t>(it);
					} break;
					case ILInstruction::woffset8: {
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::woffset16: {
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::woffset32: {
						ILBlock::read_data<std::uint32_t>(it);
					} break;
					case ILInstruction::constref: {
						std::uint32_t cid = ILBlock::read_data<std::uint32_t>(it);
						used_constants.insert(cid);
						ILSize& s = parent->constant_memory[cid].first;
						if (s.type == ILSizeType::table) {
							used_tables.insert(s.value);
						} else if (s.type == ILSizeType::array) {
							used_arrays.insert(s.value);
						}
					} break;
					case ILInstruction::staticref: {
						std::uint32_t cid = ILBlock::read_data<std::uint32_t>(it);
						used_statics.insert(cid);
						ILSize& s = std::get<0>(parent->static_memory[cid]);
						if (s.type == ILSizeType::table) {
							used_tables.insert(s.value);
						} else if (s.type == ILSizeType::array) {
							used_arrays.insert(s.value);
						}

						std::uint32_t fid = std::get<2>(parent->static_memory[cid]);
						if (fid != UINT32_MAX) {
							used_functions.insert(fid);
							ILBytecodeFunction* fun = (ILBytecodeFunction*)parent->functions[fid].get();
							fun->clean_prepass(used_functions, used_constants, used_statics, used_vtables, used_decls, used_tables, used_arrays);
						}
					} break;
					case ILInstruction::roffset32: {
						ILBlock::read_data<ILDataTypePair>(it);
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					case ILInstruction::roffset16: {
						ILBlock::read_data<ILDataTypePair>(it);
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					case ILInstruction::roffset8: {
						ILBlock::read_data<ILDataTypePair>(it);
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					case ILInstruction::aroffset: {
						ILBlock::read_data<ILDataTypePair>(it);
						ILBlock::read_data<std::uint8_t>(it);
					} break;

					case ILInstruction::wroffset: {
						ILBlock::read_data<ILDataTypePair>(it);
						ILBlock::read_data<std::uint8_t>(it);
					} break;

					case ILInstruction::local8: {
						ILBlock::read_data<std::uint8_t>(it);
					} break;

					case ILInstruction::local16: {
						ILBlock::read_data<std::uint16_t>(it);
					} break;

					case ILInstruction::local32: {
						ILBlock::read_data<std::uint32_t>(it);
					} break;

					case ILInstruction::table8offset8: {
						used_tables.insert(ILBlock::read_data<std::uint8_t>(it));
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::table8offset16: {
						used_tables.insert(ILBlock::read_data<std::uint8_t>(it));
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::table8offset32: {
						used_tables.insert(ILBlock::read_data<std::uint8_t>(it));
						ILBlock::read_data<std::uint32_t>(it);
					} break;
					case ILInstruction::table16offset8: {
						used_tables.insert(ILBlock::read_data<std::uint16_t>(it));
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::table16offset16: {
						used_tables.insert(ILBlock::read_data<std::uint16_t>(it));
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::table16offset32: {
						used_tables.insert(ILBlock::read_data<std::uint16_t>(it));
						ILBlock::read_data<std::uint32_t>(it);
					} break;
					case ILInstruction::table32offset8: {
						used_tables.insert(ILBlock::read_data<std::uint32_t>(it));
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::table32offset16: {
						used_tables.insert(ILBlock::read_data<std::uint32_t>(it));
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::table32offset32: {
						used_tables.insert(ILBlock::read_data<std::uint32_t>(it));
						ILBlock::read_data<std::uint32_t>(it);
					} break;

					case ILInstruction::table8roffset8: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint8_t>(it));
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::table8roffset16: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint8_t>(it));
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::table8roffset32: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint8_t>(it));
						ILBlock::read_data<std::uint32_t>(it);
					} break;
					case ILInstruction::table16roffset8: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint16_t>(it));
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::table16roffset16: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint16_t>(it));
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::table16roffset32: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint16_t>(it));
						ILBlock::read_data<std::uint32_t>(it);
					} break;
					case ILInstruction::table32roffset8: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint32_t>(it));
						ILBlock::read_data<std::uint8_t>(it);
					} break;
					case ILInstruction::table32roffset16: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint32_t>(it));
						ILBlock::read_data<std::uint16_t>(it);
					} break;
					case ILInstruction::table32roffset32: {
						ILBlock::read_data<ILDataTypePair>(it);
						used_tables.insert(ILBlock::read_data<std::uint32_t>(it));
						ILBlock::read_data<std::uint32_t>(it);
					} break;

					case ILInstruction::debug: {
						ILBlock::read_data<std::uint16_t>(it);
						ILBlock::read_data<std::uint16_t>(it);
					} break;

					case ILInstruction::load: {
						ILBlock::read_data<ILDataType>(it);
					} break;

					case ILInstruction::isnotzero: {
						ILBlock::read_data<ILDataType>(it);
					} break;

					case ILInstruction::negative: {
						ILBlock::read_data<ILDataType>(it);
					} break;

					case ILInstruction::forget: {
						ILBlock::read_data<ILDataType>(it);
					} break;

					case ILInstruction::jmpz: {
						ILBlock::read_data<std::uint32_t>(it);
						ILBlock::read_data<std::uint32_t>(it);
					} break;

					case ILInstruction::u8:
					case ILInstruction::u16_8: 
					case ILInstruction::u32_8:
					case ILInstruction::u64_8: ILBlock::read_data<std::uint8_t>(it); break;
					case ILInstruction::i8: 
					case ILInstruction::i16_8: 
					case ILInstruction::i32_8:
					case ILInstruction::i64_8: ILBlock::read_data<std::int8_t>(it); break;
					case ILInstruction::u16: 
					case ILInstruction::u32_16: 
					case ILInstruction::u64_16: ILBlock::read_data<std::uint16_t>(it); break;
					case ILInstruction::i16: 
					case ILInstruction::i32_16: 
					case ILInstruction::i64_16: ILBlock::read_data<std::int16_t>(it); break;
					case ILInstruction::u32: 
					case ILInstruction::u64_32: ILBlock::read_data<std::uint32_t>(it); break;
					case ILInstruction::i32: 
					case ILInstruction::i64_32: ILBlock::read_data<std::int32_t>(it); break;
					case ILInstruction::u64: ILBlock::read_data<std::uint64_t>(it); break;
					case ILInstruction::i64: ILBlock::read_data<std::int64_t>(it); break;
					case ILInstruction::f32: ILBlock::read_data<float>(it); break;
					case ILInstruction::f64: ILBlock::read_data<double>(it); break;
					case ILInstruction::word: ILBlock::read_data<void*>(it); break;
					case ILInstruction::slice: {

						std::uint32_t cid = ILBlock::read_data<std::uint32_t>(it);
						used_constants.insert(cid);
						ILSize& s = parent->constant_memory[cid].first;
						if (s.type == ILSizeType::table) {
							used_tables.insert(s.value);
						} else if (s.type == ILSizeType::array) {
							used_arrays.insert(s.value);
						}

						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					}break;
					case ILInstruction::size8: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;
					case ILInstruction::size16: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;
					case ILInstruction::size32: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					case ILInstruction::extract8: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					
					case ILInstruction::extract16: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					
					case ILInstruction::extract32: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					
					case ILInstruction::cut8: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					
					case ILInstruction::cut16: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

					
					case ILInstruction::cut32: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						if (t == ILSizeType::table) {
							used_tables.insert(v);
						}
						else if (t == ILSizeType::array) {
							used_arrays.insert(v);
						}
					} break;

				}
			}
		}
	}







	void ILBytecodeFunction::clean_pass(std::unordered_map<std::size_t, std::size_t>& map_functions,
		std::unordered_map<std::size_t, std::size_t>& map_constants,
		std::unordered_map<std::size_t, std::size_t>& map_statics,
		std::unordered_map<std::size_t, std::size_t>& map_vtables,
		std::unordered_map<std::size_t, std::size_t>& map_decls,
		std::unordered_map<std::size_t, std::size_t>& map_tables,
		std::unordered_map<std::size_t, std::size_t>& map_arrays) {


		local_stack_lifetime.clean_pass(map_tables, map_arrays);

		std::unordered_map<std::size_t, std::size_t> map_blocks;
		std::size_t nid = 0;
		for (auto&& block : blocks) {
			map_blocks[block->id] = nid++;
		}



		for (auto&& block : blocks) {
			std::vector<std::uint8_t> original_data = std::move(block->data_pool);
			auto it = original_data.begin();
			
			while (it != original_data.end()) {

				auto inst = ILBlock::read_data<ILInstruction>(it);

				switch (inst) {
					case ILInstruction::ret: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;
					case ILInstruction::call8: {
						std::uint32_t v = (std::uint32_t)map_decls[ILBlock::read_data<std::uint8_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::call8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::call16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::call32);
							block->write_value((std::uint32_t)v);
						}
					} break;
					case ILInstruction::call16: {
						std::uint32_t v = (std::uint32_t)map_decls[ILBlock::read_data<std::uint16_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::call8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::call16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::call32);
							block->write_value((std::uint32_t)v);
						}
					} break;
					case ILInstruction::call32: {
						std::uint32_t v = (std::uint32_t)map_decls[ILBlock::read_data<std::uint32_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::call8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::call16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::call32);
							block->write_value((std::uint32_t)v);
						}
					} break;

					case ILInstruction::memcpy: {
						block->write_value(inst);
						auto s = ILBlock::read_data<ILSize>(it);
						if (s.type == ILSizeType::table) {
							s.value = (tableid_t)map_tables[s.value];
						}
						else if (s.type == ILSizeType::array) {
							s.value = (tableid_t)map_arrays[s.value];
						}
						block->write_value(s);
					} break;
					case ILInstruction::memcpy2: {
						block->write_value(inst);
						auto s = ILBlock::read_data<ILSize>(it);
						if (s.type == ILSizeType::table) {
							s.value = (tableid_t)map_tables[s.value];
						}
						else if (s.type == ILSizeType::array) {
							s.value = (tableid_t)map_arrays[s.value];
						}
						block->write_value(s);
					} break;
					case ILInstruction::memcmp: {
						block->write_value(inst);
						auto s = ILBlock::read_data<ILSize>(it);
						if (s.type == ILSizeType::table) {
							s.value = (tableid_t)map_tables[s.value];
						}
						else if (s.type == ILSizeType::array) {
							s.value = (tableid_t)map_arrays[s.value];
						}
						block->write_value(s);
					} break;
					case ILInstruction::memcmp2: {
						block->write_value(inst);
						auto s = ILBlock::read_data<ILSize>(it);
						if (s.type == ILSizeType::table) {
							s.value = (std::uint32_t)map_tables[s.value];
						}
						else if (s.type == ILSizeType::array) {
							s.value = (tableid_t)map_arrays[s.value];
						}
						block->write_value(s);
					} break;
					

					case ILInstruction::fnptr8: {
						std::uint32_t v = (std::uint32_t)map_functions[ILBlock::read_data<std::uint8_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::fnptr8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::fnptr16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::fnptr32);
							block->write_value((std::uint32_t)v);
						}
					} break;

					case ILInstruction::fnptr16: {
						std::uint32_t v = (std::uint32_t)map_functions[ILBlock::read_data<std::uint16_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::fnptr8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::fnptr16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::fnptr32);
							block->write_value((std::uint32_t)v);
						}
					} break;

					case ILInstruction::fnptr32: {
						std::uint32_t v = (std::uint32_t)map_functions[ILBlock::read_data<std::uint32_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::fnptr8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::fnptr16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::fnptr32);
							block->write_value((std::uint32_t)v);
						}
					} break;


					case ILInstruction::fncall8: {
						std::uint32_t v = (std::uint32_t)map_functions[ILBlock::read_data<std::uint8_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::fncall8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::fncall16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::fncall32);
							block->write_value((std::uint32_t)v);
						}
					} break;

					case ILInstruction::fncall16: {
						std::uint32_t v = (std::uint32_t)map_functions[ILBlock::read_data<std::uint16_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::fncall8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::fncall16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::fncall32);
							block->write_value((std::uint32_t)v);
						}
					} break;

					case ILInstruction::fncall32: {
						std::uint32_t v = (std::uint32_t)map_functions[ILBlock::read_data<std::uint32_t>(it)];
						if (v<=UINT8_MAX) {
							block->write_value(ILInstruction::fncall8);
							block->write_value((std::uint8_t)v);
						} else if (v<=UINT16_MAX) {
							block->write_value(ILInstruction::fncall16);
							block->write_value((std::uint16_t)v);
						} else {
							block->write_value(ILInstruction::fncall32);
							block->write_value((std::uint32_t)v);
						}
					} break;
					
					case ILInstruction::vtable: {
						block->write_value(inst);
						block->write_value((std::uint32_t)map_vtables[ILBlock::read_data<std::uint32_t>(it)]);
					} break;
					case ILInstruction::duplicate: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;
					case ILInstruction::clone: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::swap: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;
					case ILInstruction::swap2: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataTypePair>(it));
					} break;
					case ILInstruction::insintric: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::rmemcmp: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;
					case ILInstruction::rmemcmp2: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;
					case ILInstruction::sub:
					case ILInstruction::div:
					case ILInstruction::rem:
					case ILInstruction::mul:
					case ILInstruction::add:
					case ILInstruction::bit_and:
					case ILInstruction::bit_or:
					case ILInstruction::bit_xor:
					case ILInstruction::eq:
					case ILInstruction::ne:
					case ILInstruction::gt:
					case ILInstruction::lt:
					case ILInstruction::ge:
					case ILInstruction::le: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataTypePair>(it));
					} break;

					case ILInstruction::cast:
					case ILInstruction::bitcast: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataTypePair>(it));
					} break;

					case ILInstruction::store:
					case ILInstruction::store2:
					case ILInstruction::yield:
					case ILInstruction::accept:
					case ILInstruction::discard: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;

					case ILInstruction::jmp8: {
						std::uint32_t v = (std::uint32_t)map_blocks[ILBlock::read_data<std::uint8_t>(it)];
						if (v <= UINT8_MAX) {
							block->write_value(ILInstruction::jmp8);
							block->write_value((std::uint8_t)v);
						}
						else if (v <= UINT16_MAX) {
							block->write_value(ILInstruction::jmp16);
							block->write_value((std::uint16_t)v);
						}
						else {
							block->write_value(ILInstruction::jmp32);
							block->write_value((std::uint32_t)v);
						}
					} break;

					case ILInstruction::jmp16: {
						std::uint32_t v = (std::uint32_t)map_blocks[ILBlock::read_data<std::uint16_t>(it)];
						if (v <= UINT8_MAX) {
							block->write_value(ILInstruction::jmp8);
							block->write_value((std::uint8_t)v);
						}
						else if (v <= UINT16_MAX) {
							block->write_value(ILInstruction::jmp16);
							block->write_value((std::uint16_t)v);
						}
						else {
							block->write_value(ILInstruction::jmp32);
							block->write_value((std::uint32_t)v);
						}
					} break;

					case ILInstruction::jmp32: {
						std::uint32_t v = (std::uint32_t)map_blocks[ILBlock::read_data<std::uint32_t>(it)];
						if (v <= UINT8_MAX) {
							block->write_value(ILInstruction::jmp8);
							block->write_value((std::uint8_t)v);
						}
						else if (v <= UINT16_MAX) {
							block->write_value(ILInstruction::jmp16);
							block->write_value((std::uint16_t)v);
						}
						else {
							block->write_value(ILInstruction::jmp32);
							block->write_value((std::uint32_t)v);
						}
					} break;

					case ILInstruction::offset16: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::offset8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::offset16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::offset32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}
					} break;
					case ILInstruction::offset32: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::offset8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::offset16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::offset32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}
					} break;
					case ILInstruction::offset8: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::offset8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::offset16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::offset32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}
					} break;

					case ILInstruction::aoffset8: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::aoffset16: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::aoffset32: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;
					case ILInstruction::woffset8: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::woffset16: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::woffset32: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;
					case ILInstruction::constref: {
						block->write_value(inst);
						block->write_value((std::uint32_t)map_constants[ILBlock::read_data<std::uint32_t>(it)]);
					} break;
					case ILInstruction::staticref: {
						block->write_value(inst);
						block->write_value((std::uint32_t)map_statics[ILBlock::read_data<std::uint32_t>(it)]);
					} break;

					case ILInstruction::roffset32: {
						auto tp = ILBlock::read_data<ILDataTypePair>(it);
						
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::roffset8);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::roffset16);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::roffset32);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}
					} break;
					case ILInstruction::roffset16: {
						
						auto tp = ILBlock::read_data<ILDataTypePair>(it);
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::roffset8);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::roffset16);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::roffset32);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}
					} break;
					case ILInstruction::roffset8: {
						auto tp = ILBlock::read_data<ILDataTypePair>(it);
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::roffset8);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::roffset16);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::roffset32);
								block->write_value(tp);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}
					} break;

					case ILInstruction::aroffset: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataTypePair>(it));
						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;

					case ILInstruction::wroffset: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataTypePair>(it));
						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;

					case ILInstruction::local8: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;

					case ILInstruction::local16: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;

					case ILInstruction::local32: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;

					case ILInstruction::table8offset8: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint8_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset8);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset8);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset8);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::table8offset16: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint8_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset16);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset16);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset16);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::table8offset32: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint8_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset32);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset32);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset32);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;
					case ILInstruction::table16offset8: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint16_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset8);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset8);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset8);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::table16offset16: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint16_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset16);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset16);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset16);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::table16offset32: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint16_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset32);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset32);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset32);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;
					case ILInstruction::table32offset8: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint32_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset8);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset8);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset8);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::table32offset16: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint32_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset16);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset16);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset16);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::table32offset32: {
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint32_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8offset32);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16offset32);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32offset32);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;



					case ILInstruction::table8roffset8: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint8_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset8);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset8);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset8);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::table8roffset16: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint8_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset16);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset16);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset16);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::table8roffset32: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint8_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset32);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset32);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset32);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;
					case ILInstruction::table16roffset8: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint16_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset8);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset8);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset8);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::table16roffset16: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint16_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset16);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset16);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset16);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::table16roffset32: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint16_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset32);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset32);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset32);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;
					case ILInstruction::table32roffset8: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint32_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset8);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset8);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset8);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint8_t>(it));
					} break;
					case ILInstruction::table32roffset16: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint32_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset16);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset16);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset16);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;
					case ILInstruction::table32roffset32: {
						auto pair = ILBlock::read_data<ILDataTypePair>(it);
						tableid_t table = (tableid_t)map_tables[ILBlock::read_data<std::uint32_t>(it)];
						switch (bit(table))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::table8roffset32);
								block->write_value(pair);
								block->write_value((std::uint8_t)table);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::table16roffset32);
								block->write_value(pair);
								block->write_value((std::uint16_t)table);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::table32roffset32);
								block->write_value(pair);
								block->write_value((std::uint32_t)table);
							} break;
						}

						block->write_value(ILBlock::read_data<std::uint32_t>(it));
					} break;

					case ILInstruction::debug: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<std::uint16_t>(it));
						block->write_value(ILBlock::read_data<std::uint16_t>(it));
					} break;

					case ILInstruction::load: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;

					case ILInstruction::isnotzero: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;

					case ILInstruction::negative: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;

					case ILInstruction::forget: {
						block->write_value(inst);
						block->write_value(ILBlock::read_data<ILDataType>(it));
					} break;

					case ILInstruction::jmpz: {
						block->write_value(inst);
						block->write_value((std::uint32_t)map_blocks[ILBlock::read_data<std::uint32_t>(it)]);
						block->write_value((std::uint32_t)map_blocks[ILBlock::read_data<std::uint32_t>(it)]);
					} break;

					case ILInstruction::u8:  
					case ILInstruction::u16_8:
					case ILInstruction::u32_8: 
					case ILInstruction::u64_8:   block->write_value(inst); block->write_value(ILBlock::read_data<std::uint8_t>(it)); break;
					case ILInstruction::i8:
					case ILInstruction::i16_8:
					case ILInstruction::i32_8:
					case ILInstruction::i64_8:   block->write_value(inst); block->write_value(ILBlock::read_data<std::int8_t>(it)); break;
					case ILInstruction::u16: 
					case ILInstruction::u32_16:  
					case ILInstruction::u64_16:  block->write_value(inst); block->write_value(ILBlock::read_data<std::uint16_t>(it)); break;
					case ILInstruction::i16:  
					case ILInstruction::i32_16: 
					case ILInstruction::i64_16:  block->write_value(inst); block->write_value(ILBlock::read_data<std::int16_t>(it)); break;
					case ILInstruction::u32:  
					case ILInstruction::u64_32:  block->write_value(inst); block->write_value(ILBlock::read_data<std::uint32_t>(it)); break;
					case ILInstruction::i32:  
					case ILInstruction::i64_32:  block->write_value(inst); block->write_value(ILBlock::read_data<std::int32_t>(it)); break;
					case ILInstruction::u64:  block->write_value(inst); block->write_value(ILBlock::read_data<std::uint64_t>(it)); break;
					case ILInstruction::i64:  block->write_value(inst); block->write_value(ILBlock::read_data<std::int64_t>(it)); break;
					case ILInstruction::f32:  block->write_value(inst); block->write_value(ILBlock::read_data<float>(it)); break;
					case ILInstruction::f64:  block->write_value(inst); block->write_value(ILBlock::read_data<double>(it)); break;
					case ILInstruction::word: block->write_value(inst); block->write_value(ILBlock::read_data<void*>(it)); break;
					case ILInstruction::slice: {
						block->write_value(inst);
						block->write_value((std::uint32_t) map_constants[ILBlock::read_data<std::uint32_t>(it)]);
						
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);

						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						
						block->write_value(t);
						block->write_value(new_id);
					}break;

					case ILInstruction::size16: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::size8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::size16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::size32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;
					case ILInstruction::size32: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::size8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::size16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::size32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;
					case ILInstruction::size8: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::size8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::size16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::size32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;

					
					case ILInstruction::extract8: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::extract8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::extract16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::extract32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;


					case ILInstruction::extract16: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::extract8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::extract16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::extract32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;

					case ILInstruction::extract32: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::extract8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::extract16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::extract32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;
					
					case ILInstruction::cut8: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint8_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::cut8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::cut16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::cut32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;
					
					case ILInstruction::cut16: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint16_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::cut8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::cut16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::cut32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;
					
					case ILInstruction::cut32: {
						auto t = ILBlock::read_data<ILSizeType>(it);
						auto v = ILBlock::read_data<std::uint32_t>(it);
						std::uint32_t new_id = v;
						if (t == ILSizeType::table) {
							new_id = (tableid_t)map_tables[new_id];
						}
						else if (t == ILSizeType::array) {
							new_id = (tableid_t)map_arrays[new_id];
						}

						switch (bit(new_id))
						{
							case ILBitWidth::b8: {
								block->write_value(ILInstruction::cut8);
								block->write_value(t);
								block->write_value((std::uint8_t)new_id);
							} break;
							case ILBitWidth::b16: {
								block->write_value(ILInstruction::cut16);
								block->write_value(t);
								block->write_value((std::uint16_t)new_id);
							} break;
							case ILBitWidth::b32: {
								block->write_value(ILInstruction::cut32);
								block->write_value(t);
								block->write_value((std::uint32_t)new_id);
							} break;
						}

					} break;


					case ILInstruction::start: 
					case ILInstruction::rtoffset: 
					case ILInstruction::rtoffset2:
					case ILInstruction::negate: 
					case ILInstruction::null: 
					case ILInstruction::combinedw:
					case ILInstruction::panic:
					case ILInstruction::highdw:
					case ILInstruction::lowdw:
					case ILInstruction::splitdw: {
						block->write_value(inst);
					} break;



				}
			}

		}

		std::vector<std::unique_ptr<ILBlock>> new_block_memory(map_blocks.size());
		for (auto&& p : map_blocks) {
			new_block_memory[p.second] = std::move(blocks_memory[p.first]);
		}
		blocks_memory = std::move(new_block_memory);

	}


	void ILLifetime::clean_prepass(std::unordered_set<std::size_t>& used_tables,
		std::unordered_set<std::size_t>& used_arrays) {

		unsigned char* ptr = lifetime.data();
		unsigned char* end = ptr + lifetime.size();
		while (ptr != end) {
			switch (*(ILLifetimeEvent*)(ptr++))
			{
				case ILLifetimeEvent::append: {
					ILSizeType ptr_t = *(ILSizeType*)(ptr++);
					std::uint32_t ptr_val = (((std::uint32_t) * (ptr++)) << 24) | (((std::uint32_t) * (ptr++)) << 16) | (((std::uint32_t) * (ptr++)) << 8) | (((std::uint32_t) * (ptr++)));

					if (ptr_t == ILSizeType::table) {
						used_tables.insert(ptr_val);
					}
					else if (ptr_t == ILSizeType::array) {
						used_arrays.insert(ptr_val);
					}
				}break;

				default:break;
			}
		}
	}


	void ILLifetime::clean_pass(std::unordered_map<std::size_t, std::size_t>& map_tables,
		std::unordered_map<std::size_t, std::size_t>& map_arrays) {

		unsigned char* ptr = lifetime.data();
		unsigned char* end = ptr + lifetime.size();
		while (ptr != end) {
			switch (*(ILLifetimeEvent*)(ptr++))
			{
				case ILLifetimeEvent::append: {
					ILSizeType ptr_t = *(ILSizeType*)(ptr++);

					std::uint32_t ptr_val = (((std::uint32_t) * (ptr)) << 24) | (((std::uint32_t) * (ptr+1)) << 16) | (((std::uint32_t) * (ptr+2)) << 8) | (((std::uint32_t) * (ptr+3)));

					if (ptr_t == ILSizeType::table) {
						ptr_val = (std::uint32_t)map_tables[ptr_val];

					}
					else if (ptr_t == ILSizeType::array) {
						ptr_val = (std::uint32_t)map_arrays[ptr_val];
					}

					*ptr++ = (std::uint8_t)((ptr_val >> 24) & 0xFF);
					*ptr++ = (std::uint8_t)((ptr_val >> 16) & 0xFF);
					*ptr++ = (std::uint8_t)((ptr_val >> 8) & 0xFF);
					*ptr++ = (std::uint8_t)((ptr_val) & 0xFF);
				}break;

				default:break;
			}
		}
	}

	void ILSize::clean_prepass(ILModule* mod, std::set<void*>& pointer_offsets, unsigned char* ptr) {
		switch(type) {
			case ILSizeType::abs8:
				ptr += value;
				break;

			case ILSizeType::abs16:
				ptr += value*2;
				break;

			case ILSizeType::abs32:
			case ILSizeType::absf32:
				ptr += value*4;
				break;

			case ILSizeType::abs64:
			case ILSizeType::absf64:
				ptr += value*8;
				break;

			case ILSizeType::word: {
				ptr += value*sizeof(std::size_t);
			} break;
			case ILSizeType::ptr: {
				for (std::uint32_t i=0;i<value;++i) { 
					void* target = *(void**)ptr;

					pointer_offsets.insert(target);

					ptr += sizeof(void*);
				}
			} break;
			case ILSizeType::slice: {
				for (std::uint32_t i=0;i<value;++i) {
					void* target = *(void**)ptr;

					pointer_offsets.insert(target);

					ptr += sizeof(void*);
					ptr+=sizeof(std::size_t);
				}
			}break;
			case ILSizeType::table: {
				std::size_t s = eval(mod);
				auto& table = mod->structure_tables[value];
				table.calculate(mod);
				for (std::size_t e = 0; e<table.elements.size(); ++e) {
					unsigned char* elem_off = ptr + table.calculated_offsets[e];
					table.elements[e].clean_prepass(mod, pointer_offsets, elem_off);
				}
				ptr+=s;
			}
			case ILSizeType::array: {
				std::size_t s = eval(mod);
				auto& table = mod->array_tables[value];
				std::size_t es = table.element.eval(mod);
				table.calculate(mod);
				unsigned char* offset = ptr;
				for (std::uint32_t i=0; i<table.count; ++i) {
					table.element.clean_prepass(mod, pointer_offsets, offset);
					offset += es;
				}
				ptr+=s;
			}
		}
	}

	
}