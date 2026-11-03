// Base test: builds the environment and runs a sequence on the SRAM agent.
// The sequence to run is selected by name through a plusarg (+SEQ_NAME=...);
// the factory is used to override dyn_ecc_base_seq (by type and by instance)
// with that named sequence. With no plusarg, the empty base sequence runs.
class dyn_ecc_base_test extends uvm_test;

  `uvm_component_utils(dyn_ecc_base_test)

  dyn_ecc_env env;
  string      seq_name = "dyn_ecc_base_seq";

  function new(string name, uvm_component parent);
    super.new(name, parent);
  endfunction

  function void build_phase(uvm_phase phase);
    uvm_coreservice_t cs = uvm_coreservice_t::get();
    uvm_factory       f  = cs.get_factory();

    super.build_phase(phase);

    // Select the sequence by name from the command line.
    void'($value$plusargs("SEQ_NAME=%s", seq_name));

    // Replace the base sequence with the requested one, by type and instance.
    if (seq_name != "dyn_ecc_base_seq") begin
      f.set_type_override_by_name("dyn_ecc_base_seq", seq_name);
      f.set_inst_override_by_name("dyn_ecc_base_seq", seq_name, "*");
      `uvm_info(get_type_name(),
                $sformatf("Sequence overridden by name: %s", seq_name), UVM_LOW)
    end

    env = dyn_ecc_env::type_id::create("env", this);
  endfunction

  virtual task run_phase(uvm_phase phase);
    dyn_ecc_base_seq seq;
    phase.raise_objection(this, "running sequence");
    // Factory returns the overridden type if one was registered.
    seq = dyn_ecc_base_seq::type_id::create("seq");
    seq.start(env.mcu_agt.seqr);
    phase.drop_objection(this, "sequence done");
  endtask

endclass
