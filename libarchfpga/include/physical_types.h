/*
 Data types describing the physical components on the FPGA architecture.

 We assume an island style FPGA where complex logic blocks are arranged in a grid and each side of the logic block has access to the inter-block interconnect.  To keep the logic blocks general,
 we allow arbitrary hierarchy, modes, primitives, and interconnect within each complex logic block.  The data structures here describe the properties of the island-style FPGA as well as the details on
 hierarchy, modes, primitives, and intconnect within each logic block.

 Data structures that flesh out 

 The data structures that store the 

 Key data types:
 t_type_descriptor: describes a placeable complex logic block, 
 pb_type: describes the types of physical blocks within the t_type_descriptor in a hierarchy where the top block is the complex block and the leaf blocks implement one logical block
 pb_graph_node: is a flattened version of pb_type so a pb_type with 10 instances will have 10 pb_graph_nodes representing each instance
 
 Additional notes:

 The interconnect specified in the architecture file gets flattened out in the pb_graph_node netlist.  Each pb_graph_node contains pb_graph_pins which allow it to connect to other pb_graph_nodes.  
 These pins are in connected to other pins through pb_graph_edges. The pin connections are based on what is specified in the <interconnect> tags of the architecture file.  

 Date: February 19, 2009
 Authors: Jason Luu and Kenneth Kent
 */

#ifndef PHYSICAL_TYPES_H
#define PHYSICAL_TYPES_H

#include <functional>
#include <vector>
#include <string>
#include <map>
#include "logic_types.h"

//Forward declarations
typedef struct s_clock_arch t_clock_arch;
typedef struct s_clock_network t_clock_network;
typedef struct s_power_arch t_power_arch;
typedef struct s_interconnect_pins t_interconnect_pins;
typedef struct s_power_usage t_power_usage;
typedef struct s_pb_type_power t_pb_type_power;
typedef struct s_mode_power t_mode_power;
typedef struct s_interconnect_power t_interconnect_power;
typedef struct s_port_power t_port_power;
typedef struct s_pb_graph_pin_power t_pb_graph_pin_power;
typedef struct s_mode t_mode;
typedef struct s_pb_graph_node_power t_pb_graph_node_power;
typedef struct s_port t_port;
typedef struct s_pb_graph_node t_pb_graph_node;
typedef struct s_pin_to_pin_annotation t_pin_to_pin_annotation;
typedef struct s_interconnect t_interconnect;
typedef struct s_pb_graph_pin t_pb_graph_pin;
typedef struct s_pb_graph_edge t_pb_graph_edge;

/*************************************************************************************************/
/* FPGA basic definitions                                                                        */
/*************************************************************************************************/

/* Pins describe I/O into clustered logic block.  
 A pin may be unconnected, driving a net or in the fanout, respectively. */
enum e_pin_type {
	OPEN = -1, DRIVER = 0, RECEIVER = 1
};

/* Type of interconnect within complex block: Complete for everything connected (full crossbar), direct for one-to-one connections, and mux for many-to-one connections */
enum e_interconnect {
	COMPLETE_INTERC = 1, DIRECT_INTERC = 2, MUX_INTERC = 3
};

/* Orientations. */
enum e_side {
	TOP = 0, RIGHT = 1, BOTTOM = 2, LEFT = 3
};

/* pin location distributions */
enum e_pin_location_distr {
	E_SPREAD_PIN_DISTR = 1, E_CUSTOM_PIN_DISTR = 2
};

/* pb_type class */
enum e_pb_type_class {
	UNKNOWN_CLASS = 0, LUT_CLASS = 1, LATCH_CLASS = 2, MEMORY_CLASS = 3
};

/* Annotations for pin-to-pin connections */
enum e_pin_to_pin_annotation_type {
	E_ANNOT_PIN_TO_PIN_DELAY = 0,
	E_ANNOT_PIN_TO_PIN_CAPACITANCE,
	E_ANNOT_PIN_TO_PIN_PACK_PATTERN
};
enum e_pin_to_pin_annotation_format {
	E_ANNOT_PIN_TO_PIN_MATRIX = 0, E_ANNOT_PIN_TO_PIN_CONSTANT
};
enum e_pin_to_pin_delay_annotations {
	E_ANNOT_PIN_TO_PIN_DELAY_MIN = 0,
	E_ANNOT_PIN_TO_PIN_DELAY_MAX,
	E_ANNOT_PIN_TO_PIN_DELAY_TSETUP,
	E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN,
	E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX,
	E_ANNOT_PIN_TO_PIN_DELAY_THOLD
};
enum e_pin_to_pin_capacitance_annotations {
	E_ANNOT_PIN_TO_PIN_CAPACITANCE_C = 0
};
enum e_pin_to_pin_pack_pattern_annotations {
	E_ANNOT_PIN_TO_PIN_PACK_PATTERN_NAME = 0
};

/* Power Estimation type for a PB */
enum e_power_estimation_method_ {
	POWER_METHOD_UNDEFINED = 0, POWER_METHOD_IGNORE, /* Ignore power of this PB, and all children PB */
	POWER_METHOD_SUM_OF_CHILDREN, /* Ignore power of this PB, but consider children */
	POWER_METHOD_AUTO_SIZES, /* Transistor-level, auto-sized buffers/wires */
	POWER_METHOD_SPECIFY_SIZES, /* Transistor-level, user-specified buffers/wires */
	POWER_METHOD_TOGGLE_PINS, /* Dynamic: Energy per pin toggle, Static: Absolute */
	POWER_METHOD_C_INTERNAL, /* Dynamic: Equiv. Internal capacitance, Static: Absolute */
	POWER_METHOD_ABSOLUTE /* Dynamic: Aboslute, Static: Absolute */
};
typedef enum e_power_estimation_method_ e_power_estimation_method;
typedef enum e_power_estimation_method_ t_power_estimation_method;

/* Specifies what part of the FPGA a custom switchblock should be built in (i.e. perimeter, core, everywhere) */
enum e_sb_location{
	E_PERIMETER = 0,
	E_CORNER,
	E_FRINGE,		/* perimeter minus corners */
	E_CORE,
	E_EVERYWHERE
};

/*************************************************************************************************/
/* FPGA grid layout data types                                                                   */
/*************************************************************************************************/
/* Definition of how to place physical logic block in the grid 
 grid_loc_type - where the type goes and which numbers are valid
 start_col - the absolute value of the starting column from the left to fill, 
 used with COL_REPEAT
 repeat - the number of columns to skip before placing the same type, 
 used with COL_REPEAT.  0 means do not repeat
 rel_col - the fractional column to place type
 priority - in the event of conflict, which type gets picked?
 */
enum e_grid_loc_type {
	BOUNDARY = 0, FILL, COL_REPEAT, COL_REL
};
typedef struct s_grid_loc_def {
	enum e_grid_loc_type grid_loc_type;
	int start_col;
	int repeat;
	float col_rel;
	int priority;
} t_grid_loc_def;

/* Data type definitions */
/*   Grid info */
struct s_clb_grid {
	bool IsAuto;
	float Aspect;
	int W;
	int H;
};

/************************* POWER ***********************************/

/* Global clock architecture */
struct s_clock_arch {
	int num_global_clocks;
	t_clock_network *clock_inf; /* Details about each clock */
};

/* Architecture information for a single clock */
struct s_clock_network {
	bool autosize_buffer; /* autosize clock buffers */
	float buffer_size; /* if not autosized, the clock buffer size */
	float C_wire; /* Wire capacitance (per meter) */

	float prob; /* Static probability of net assigned to this clock */
	float dens; /* Switching density of net assigned to this clock */
	float period; /* Period of clock */
};

/* Power-related architecture information */
struct s_power_arch {
	float C_wire_local; /* Capacitance of local interconnect (per meter) */
	//int seg_buffer_split; /* Split segment for distributed buffer (no split=1) */
	float logical_effort_factor;
	float local_interc_factor;
	float transistors_per_SRAM_bit;
	float mux_transistor_size;
	float FF_size;
	float LUT_transistor_size;
};

/* Power usage for an entity */
struct s_power_usage {
	float dynamic;
	float leakage;
};

/*************************************************************************************************/
/* FPGA Physical Logic Blocks data types                                                         */
/*************************************************************************************************/

/* A class of CLB pins that share common properties
 * port_name: name of this class of pins
 * type:  DRIVER or RECEIVER (what is this pinclass?)              *
 * num_pins:  The number of logically equivalent pins forming this *
 *           class.                                                *
 * pinlist[]:  List of clb pin numbers which belong to this class. */
struct s_class {
	enum e_pin_type type;
	int num_pins;
	int *pinlist; /* [0..num_pins - 1] */
};
typedef struct s_class t_class;

/* Cluster timing delays:
 * C_ipin_cblock: Capacitance added to a routing track by the isolation     *
 *                buffer between a track and the Cblocks at an (i,j) loc.   *
 * T_ipin_cblock: Delay through an input pin connection box (from a         *
 *                   routing track to a logic block input pin).             */
typedef struct s_timing_inf {
	bool timing_analysis_enabled;
	float C_ipin_cblock;
	float T_ipin_cblock;
	char * SDCFile; 
    char slack_definition;
} t_timing_inf;

typedef enum {
	POWER_WIRE_TYPE_UNDEFINED = 0,
	POWER_WIRE_TYPE_IGNORED,
	POWER_WIRE_TYPE_C,
	POWER_WIRE_TYPE_ABSOLUTE_LENGTH,
	POWER_WIRE_TYPE_RELATIVE_LENGTH,
	POWER_WIRE_TYPE_AUTO
} e_power_wire_type;

typedef enum {
	POWER_BUFFER_TYPE_UNDEFINED = 0,
	POWER_BUFFER_TYPE_NONE,
	POWER_BUFFER_TYPE_AUTO,
	POWER_BUFFER_TYPE_ABSOLUTE_SIZE
} e_power_buffer_type;

struct s_port_power {
	/* Transistor-Level Power Properties */

	// Wire
	e_power_wire_type wire_type;
	union {
		float C;
		float absolute_length;
		float relative_length;
	} wire;

	// Buffer
	e_power_buffer_type buffer_type;
	float buffer_size;

	/* Pin-Toggle Power Properties */
	bool pin_toggle_initialized;
	float energy_per_toggle;
	t_port * scaled_by_port;
	int scaled_by_port_pin_idx;
	bool reverse_scaled; /* Scale by (1-prob) */
};

/* Describes the type for a physical logic block
 name: unique identifier for type  
 num_pins: Number of pins for the block
 capacity: Number of blocks of this type that can occupy one grid tile.
 This is primarily used for IO pads.
 width: Width of large block in grid tiles
 height: Height of large block in grid tiles
 pinloc: Is set to 1 if a given pin exists on a certain position of a block.
 num_class: Number of logically-equivalent pin classes
 class_inf: Information of each logically-equivalent class
 pin_class: The class a pin belongs to
 is_global_pin: Whether or not a pin is global (hence not routed)
 is_Fc_frac: true if Fc fractional, else Fc absolute
 is_Fc_out_full_flex: true means opins will connect to all available segments
 pb_type: Internal subblocks and routing information for this physical block
 pb_graph_head: Head of DAG of pb_types_nodes and their edges

 area: Describes how much area this logic block takes, if undefined, use default
 type_timing_inf: timing information unique to this type
 num_drivers: Total number of output drivers supplied
 num_receivers: Total number of input receivers supplied
 index: Keep track of type in array for easy access
 */
struct s_type_descriptor /* TODO rename this.  maybe physical type descriptor or complex logic block or physical logic block*/
{
	char *name;
	int num_pins;
	int capacity;

	int width;
	int height;

	int ****pinloc; /* [0..width-1][0..height-1][0..3][0..num_pins-1] */
	int *pin_width; /* [0..num_pins-1] */
	int *pin_height; /* [0..num_pins-1] */
	int ***num_pin_loc_assignments; /* [0..width-1][0..height-1][0..3] */
	char *****pin_loc_assignments; /* [0..width-1][0..height-1][0..3][0..num_tokens-1][0..string_name] */
	enum e_pin_location_distr pin_location_distribution;

	int num_class;
	struct s_class *class_inf; /* [0..num_class-1] */
	int *pin_class; /* [0..num_pins-1] */

	bool *is_global_pin; /* [0..num_pins-1] */

	bool *is_Fc_frac; /* [0..num_pins-1] */
	bool *is_Fc_full_flex; /* [0..num_pins-1] */
	float **Fc; /* [0..num_pins-1][0..num_segments-1] where num_segments is specified in s_arch or s_det_routing_arch */

	/* Clustering info */
	struct s_pb_type *pb_type;
	t_pb_graph_node *pb_graph_head;

	/* Grid location info */
	struct s_grid_loc_def *grid_loc_def; /* [0..num_def-1] */
	int num_grid_loc_def;
	float area;

	/* This info can be determined from class_inf and pin_class but stored for faster access */
	int num_drivers;
	int num_receivers;

	int index; /* index of type descriptor in array (allows for index referencing) */
};
typedef struct s_type_descriptor t_type_descriptor;
typedef const struct s_type_descriptor *t_type_ptr;

/*************************************************************************************************
 * PB Type Hierarchy                                                                             *
 *************************************************************************************************
 *
 * VPR represents the 'type' of block types corresponding to FPGA grid locations using a hierarchy
 * of t_pb_type objects.
 *
 * The root t_pb_type corresponds to a single top level block type and maps to a particular type 
 * of location in the FPGA device grid (e.g. Logic, DSP, RAM etc.).
 * 
 * A non-root t_pb_type represents an intermediate level of hierarchy within the root block type.
 *
 * The PB Type hierarchy corresponds to the tags specified in the FPGA architecture description:
 *
 *      struct              XML Tag
 *      ------              ------------
 *      t_pb_type           <pb_type/>
 *      t_mode              <mode/>
 *      t_interconnect      <interconnect/>
 *      t_port              <port/>
 *
 * The various structures hold pointers to each other which encode the hierarchy.
 */

/** Describes the type of clustered block if a root (parent_mode == nullptr), an
 *  intermediate level of hierarhcy (parent_mode != nullptr), or a leaf/primtive
 *  (num_modes == 0, model != nullptr).
 *  
 *  This (along with t_mode) corresponds to the hierarchical specification of 
 *  block modes that users provide in the architecture (i.e. <pb_type/> tags).
 *
 *  It is also useful to note that a single t_pb_type may represent multiple instances of that
 *  type in the architecture (see the num_pb field).
 *
 *  In VPR there is a single instance of a t_pb_type for each type, which is referenced as a 
 *  flyweight by other objects (e.g. t_pb_graph_node).
 *
 *  Data members:
 *      name: name of the physical block type
 *      num_pb: maximum number of instances of this physical block type sharing one parent
 *      blif_model: the string in the blif circuit that corresponds with this pb type
 *      class_type: Special library name
 *      modes: Different modes accepted
 *      ports: I/O and clock ports
 *      num_clock_pins: A count of the total number of clock pins
 *      int num_input_pins: A count of the total number of input pins
 *      int num_output_pins: A count of the total number of output pins
 *      timing: Timing matrix of block [0..num_inputs-1][0..num_outputs-1]
 *      parent_mode: mode of the parent block
 */
struct s_pb_type {
	char* name;
	int num_pb;
	char *blif_model;
	t_model *model;
	enum e_pb_type_class class_type;

	t_mode *modes; /* [0..num_modes-1] */
	int num_modes;
	t_port *ports; /* [0..num_ports] */
	int num_ports;

	int num_clock_pins;
	int num_input_pins; /* inputs not including clock pins */
	int num_output_pins;

	t_mode *parent_mode;
	int depth; /* depth of pb_type */

	float max_internal_delay;
	t_pin_to_pin_annotation *annotations; /* [0..num_annotations-1] */
	int num_annotations;

	/* Power related members */
	t_pb_type_power * pb_type_power;
};
typedef struct s_pb_type t_pb_type;

/** Describes an operational mode of a clustered logic block
 *
 *  This forms part of the t_pb_type hierarchical description of a clustered logic block.
 *  It corresponds to <mode/> tags in the FPGA architecture description
 * 
 *  Data members:
 *      name: name of the mode
 *      pb_type_children: pb_types it contains
 *      interconnect: interconnect of parent pb_type to children pb_types or children to children pb_types
 *      num_interconnect: Total number of interconnect tags specified by user
 *      parent_pb_type: Which parent contains this mode
 *      index: Index of mode in array with other modes
 */
struct s_mode {
	char* name;
	struct s_pb_type *pb_type_children; /* [0..num_child_pb_types] */
	int num_pb_type_children;
	t_interconnect *interconnect;
	int num_interconnect;
	struct s_pb_type *parent_pb_type;
	int index;

	/* Power releated members */
	t_mode_power * mode_power;
};

/** Describes an interconnect edge inside a cluster
 *
 *  This forms part of the t_pb_type hierarchical description of a clustered logic block.
 *  It corresponds to <interconnect/> tags in the FPGA archtiecture description
 *  
 *  Data members:
 *      type: type of the interconnect
 *      name: indentifier for interconnect
 *      input_string: input string verbatim to parse later
 *      output_string: input string output to parse later
 *      annotations: Annotations for delay, power, etc
 *      num_annotations: Total number of annotations
 *      infer_annotations: This interconnect is autogenerated, if true, infer pack_patterns 
 *                         such as carry-chains and forced packs based on interconnect linked to it
 *      parent_mode_index: Mode of parent as int
 */
struct s_interconnect {
	enum e_interconnect type;
	char *name;

	char *input_string;
	char *output_string;

	t_pin_to_pin_annotation *annotations; /* [0..num_annotations-1] */
	int num_annotations;
	bool infer_annotations;

	int line_num; /* Interconnect is processed later, need to know what line number it messed up on to give proper error message */

	int parent_mode_index;

	/* Power related members */
	t_mode * parent_mode;

	t_interconnect_power * interconnect_power;
};

/** Describes I/O and clock ports
 *
 *  This forms part of the t_pb_type hierarchical description of a clustered logic block.
 *  It corresponds to <port/> tags in the FPGA archtiecture description
 *
 *  Data members:
 *      name: name of the port
 *      model_port: associated model port
 *      is_clock: whether or not this port is a clock
 *      is_non_clock_global: Applies to top level pb_type, this pin is not a clock but 
 *                           is a global signal (useful for stuff like global reset signals,
 *                           perhaps useful for VCC and GND)
 *      num_pins: the number of pins this port has
 *      parent_pb_type: pointer to the parent pb_type
 *      port_class: port belongs to recognized set of ports in class library
 *      index: port index by index in array of parent pb_type
 *      port_index_by_type index of port by type (index by input, output, or clock)
 *      equivalence: Applies to logic block I/Os and to primitive inputs only
 */
struct s_port {
	char* name;
	t_model_ports *model_port;
	enum PORTS type;
	bool is_clock;
	bool is_non_clock_global;
	int num_pins;
	bool equivalent;
	struct s_pb_type *parent_pb_type;
	char * port_class;

	int index;
	int port_index_by_type;
	char *chain_name;

	t_port_power * port_power;
};

struct s_pb_type_power {
	/* Type of power estimation for this pb */
	e_power_estimation_method estimation_method;

	t_power_usage absolute_power_per_instance; /* User-provided absolute power per block */

	float C_internal; /*Internal capacitance of the pb */
	int leakage_default_mode; /* Default mode for leakage analysis, if block has no set mode */

	t_power_usage power_usage; /* Total power usage of this pb type */
	t_power_usage power_usage_bufs_wires; /* Power dissipated in local buffers and wire switching (Subset of total power) */
};

struct s_interconnect_power {

	t_power_usage power_usage;

	/* These are not necessarily power-related; however, at the moment
	 * only power estimation uses them
	 */
	bool port_info_initialized;
	int num_input_ports;
	int num_output_ports;
	int num_pins_per_port;
	float transistor_cnt;
};

struct s_interconnect_pins {
	t_interconnect * interconnect;

	struct s_pb_graph_pin *** input_pins; // [0..num_input_ports-1][0..num_pins_per_port-1]
	struct s_pb_graph_pin *** output_pins; // [0..num_output_ports-1][0..num_pins_per_port-1]
};

struct s_mode_power {
	t_power_usage power_usage; /* Power usage of this mode */
};

/** Info placed between pins in the architecture file (e.g. delay annoatations),
 * 
 * This is later for additional information.
 *
 * Data Members:
 *      value: value/property pair
 *      prop: value/property pair
 *      type: type of annotation
 *      format: formatting of data
 *      input_pins: input pins as string affected by annotation
 *      output_pins: output pins as string affected by annotation
 *      clock_pin: clock as string affected by annotation
 */
struct s_pin_to_pin_annotation {
	char ** value; /* [0..num_value_prop_pairs - 1] */
	int * prop; /* [0..num_value_prop_pairs - 1] */
	int num_value_prop_pairs;

	enum e_pin_to_pin_annotation_type type;
	enum e_pin_to_pin_annotation_format format;

	char * input_pins;
	char * output_pins;
	char * clock;

	int line_num; /* used to report what line number this annotation is found in architecture file */
};

/*************************************************************************************************
 * PB Graph                                                                                      *
 *************************************************************************************************
 *
 * The PB graph represents the flattened and elaborated connectivity within a t_pb_type (i.e. 
 * the routing resource graph), derived from the t_pb_type hierarchy.
 *
 * The PB graph is built of t_pb_graph_node and t_pb_graph_pin objects.
 *
 * There is a single PB graph associated with each root t_pb_type, and it is referenced in other objects (e.g.
 * t_pb) as a flyweight.
 *
 */

/** Describes the internal connectivity corresponding to a t_pb_type and t_mode of a cluster.
 *
 *  There is a t_pb_graph_node for each instance of the pb_type (i.e. t_pb_type may describe 
 *  num_pb instances of the type, with each instance represented as a t_pb_graph_node). 
 *  The distinction between the pb_type and the pb_graph_node is neccessary since the 'position'
 *  of a particular instance in the cluster is important when routing the cluster (since the routing
 *  accessible from each position may be different).
 *
 *  Data members:
 *      pb_type: Pointer to the type of pb graph node this belongs to 
 *      mode: parent mode of operation
 *      placement_index: there are a certain number of pbs available, this gives the index of the node
 *      child_pb_graph_nodes: array of children pb graph nodes organized into modes
 *      parent_pb_graph_node: parent pb graph node
 */
struct s_pb_graph_node {
	struct s_pb_type *pb_type;

	int placement_index;

	t_pb_graph_pin **input_pins; /* [0..num_input_ports-1] [0..num_port_pins-1]*/
	t_pb_graph_pin **output_pins; /* [0..num_output_ports-1] [0..num_port_pins-1]*/
	t_pb_graph_pin **clock_pins; /* [0..num_clock_ports-1] [0..num_port_pins-1]*/

	int num_input_ports;
	int num_output_ports;
	int num_clock_ports;

	int *num_input_pins; /* [0..num_input_ports - 1] */
	int *num_output_pins; /* [0..num_output_ports - 1] */
	int *num_clock_pins; /* [0..num_clock_ports - 1] */

	struct s_pb_graph_node ***child_pb_graph_nodes; /* [0..num_modes-1][0..num_pb_type_in_mode-1][0..num_pb-1] */
	struct s_pb_graph_node *parent_pb_graph_node;

	int total_pb_pins; /* only valid for top-level */

	void *temp_scratch_pad; /* temporary data, useful for keeping track of things when traversing data structure */
	struct s_cluster_placement_primitive *cluster_placement_primitive; /* pointer to indexing structure useful during packing stage */

	int *input_pin_class_size; /* Stores the number of pins that belong to a particular input pin class */
	int num_input_pin_class; /* number of pin classes that this input pb_graph_node has */
	int *output_pin_class_size; /* Stores the number of pins that belong to a particular output pin class */
	int num_output_pin_class; /* number of output pin classes that this pb_graph_node has */

	/* Interconnect instances for this pb
	 * Only used for power
	 */
	t_pb_graph_node_power * pb_node_power;
	t_interconnect_pins ** interconnect_pins; /* [0..num_modes-1][0..num_interconnect_in_mode] */
};


/* Identify pb pin type for timing purposes */
enum e_pb_graph_pin_type {
	PB_PIN_NORMAL = 0,
	PB_PIN_SEQUENTIAL,
	PB_PIN_INPAD,
	PB_PIN_OUTPAD,
	PB_PIN_TERMINAL,
	PB_PIN_CLOCK
};

/** Describes a pb graph pin
 *
 *  Data Members:
 *      port: pointer to the port that this pin is associated with 
 *      pin_number: pin number of the port that this pin is associated with
 *      input edges: [0..num_input_edges - 1]edges incoming
 *      num_input_edges: number edges incoming
 *      output edges: [0..num_output_edges - 1]edges out_going
 *      num_output_edges: number edges out_going
 *      parent_node: parent pb_graph_node
 *      pin_count_in_cluster: Unique number for pin inside cluster
 */
struct s_pb_graph_pin {
	t_port *port;
	int pin_number;
	struct s_pb_graph_edge** input_edges; /* [0..num_input_edges] */
	int num_input_edges;
	struct s_pb_graph_edge** output_edges; /* [0..num_output_edges] */
	int num_output_edges;

	struct s_pb_graph_node *parent_node;
	int pin_count_in_cluster;

	int scratch_pad; /* temporary data structure useful to store traversal info */

	/* timing information */
	enum e_pb_graph_pin_type type; /* Is a sequential logic element (true), inpad/outpad (true), or neither (false) */
	float tsu_tco; /* For sequential logic elements, this is the setup time (if input) or clock-to-q time (if output) */
    struct s_pb_graph_pin* associated_clock_pin; /* For sequentail elements, the associated clock */
	struct s_pb_graph_pin** pin_timing; /* primitive ipin to opin timing */
	float *pin_timing_del_max; /* primitive ipin to opin timing */
	int num_pin_timing; /* primitive ipin to opin timing */

	/* Applies to clusters only */
	int pin_class;

	/* Applies to pins of primitive only */
	int *parent_pin_class; /* [0..depth-1] the grouping of pins that this particular pin belongs to */
	/* Applies to output pins of primitives only */
	struct s_pb_graph_pin ***list_of_connectable_input_pin_ptrs; /* [0..depth-1][0..num_connectable_primtive_input_pins-1] what input pins this output can connect to without exiting cluster at given depth */
	int *num_connectable_primtive_input_pins; /* [0..depth-1] number of input pins that this output pin can reach without exiting cluster at given depth */

	bool is_forced_connection; /* This output pin connects to one and only one input pin */

	t_pb_graph_pin_power * pin_power;
};


/** Describes a pb graph edge
 *
 *  Note that this is a "fat" edge which supports bused based connections
 *
 *  Data members:
 *      input_pins: array of pb_type graph input pins ptrs entering this edge
 *      num_input_pins: Number of input pins entering this edge
 *      output_pins: array of pb_type graph output pins ptrs exiting this edge
 *      num_output_pins: Number of output pins exiting this edge
 */
struct s_pb_graph_edge {
	t_pb_graph_pin **input_pins;
	int num_input_pins;
	t_pb_graph_pin **output_pins;
	int num_output_pins;

	/* timing information */
	float delay_max;
	float delay_min;
	float capacitance;

	/* who drives this edge */
	t_interconnect * interconnect;
	int driver_set;
	int driver_pin;

	/* pack pattern info */
	const char **pack_pattern_names; /*[0..num_pack_patterns(of_edge)-1]*/
	int *pack_pattern_indices; /*[0..num_pack_patterns(of_edge)-1]*/
	int num_pack_patterns;
	bool infer_pattern; /*If true, infer pattern based on patterns connected to it*/
};

struct s_pb_graph_node_power {
	float transistor_cnt_pb_children; /* Total transistor size of this pb */
	float transistor_cnt_interc; /* Total transistor size of the interconnect in this pb */
	float transistor_cnt_buffers;
};

struct s_pb_graph_pin_power {
	/* Transistor-level Power Properties */
	float C_wire;
	float buffer_size;

	/* Pin-Toggle Power Properties */
	t_pb_graph_pin * scaled_by_pin;
};


/*************************************************************************************************/
/* FPGA Routing architecture                                                                     */
/*************************************************************************************************/

/* Description of routing channel distribution across the FPGA, only available for global routing
 * Width is standard dev. for Gaussian.  xpeak is where peak     *
 * occurs. dc is the dc offset for Gaussian and pulse waveforms. */
enum e_stat {
	UNIFORM, GAUSSIAN, PULSE, DELTA
};
typedef struct s_chan {
	enum e_stat type;
	float peak;
	float width;
	float xpeak;
	float dc;
} t_chan;

/* chan_width_io:  The relative width of the I/O channel between the pads    *
 *                 and logic array.                                          *
 * chan_x_dist: Describes the x-directed channel width distribution.         *
 * chan_y_dist: Describes the y-directed channel width distribution.         */
typedef struct s_chan_width_dist {
	float chan_width_io;
	t_chan chan_x_dist;
	t_chan chan_y_dist;
} t_chan_width_dist;

enum e_directionality {
	UNI_DIRECTIONAL, BI_DIRECTIONAL
};
enum e_switch_block_type {
	SUBSET, WILTON, UNIVERSAL, FULL, CUSTOM
};
typedef enum e_switch_block_type t_switch_block_type;
enum e_Fc_type {
	ABSOLUTE, FRACTIONAL
};

/* Lists all the important information about a certain segment type.  Only   *
 * used if the route_type is DETAILED.  [0 .. det_routing_arch.num_segment]  *
 * name: the name of this segment                                            *
 * frequency:  ratio of tracks which are of this segment type.               *
 * length:     Length (in clbs) of the segment.                              *
 * arch_wire_switch: Index of the switch type that connects other wires      *
 *                   *to* this segment. Note that this index is in relation  *
 *                   to the switches from the architecture file, not the     *
 *                   expanded list of switches that is built at the end of   *
 *                   build_rr_graph.                                         *
 * arch_opin_switch: Index of the switch type that connects output pins      *
 *                   (OPINs) *to* this segment. Note that this index is in   *
 *                   relation to the switches from the architecture file,    *
 *                   not the expanded list of switches that is built         *
 *                   at the end of build_rr_graph                            *
 * frac_cb:  The fraction of logic blocks along its length to which this     *
 *           segment can connect.  (i.e. internal population).               *
 * frac_sb:  The fraction of the length + 1 switch blocks along the segment  *
 *           to which the segment can connect.  Segments that aren't long    *
 *           lines must connect to at least two switch boxes.                *
 * Cmetal: Capacitance of a routing track, per unit logic block length.      *
 * Rmetal: Resistance of a routing track, per unit logic block length.       *
 * (UDSD by AY) drivers: How do signals driving a routing track connect to   *
 *                       the track?                                          */
typedef struct s_segment_inf {
	char *name;
	int frequency;
	int length;
	short arch_wire_switch;
	short arch_opin_switch;
	float frac_cb;
	float frac_sb;
	bool longline;
	float Rmetal;
	float Cmetal;
	enum e_directionality directionality;
	bool *cb;
	int cb_len;
	bool *sb;
	int sb_len;
	//float Cmetal_per_m; /* Wire capacitance (per meter) */
} t_segment_inf;

/* Lists all the important information about a switch type read from the     *
 * architecture file.                                                        *
 * [0 .. Arch.num_switch]                                                    *
 * buffered:  Does this switch include a buffer?                             *
 * R:  Equivalent resistance of the buffer/switch.                           *
 * Cin:  Input capacitance.                                                  *
 * Cout:  Output capacitance.                                                *
 * Tdel_map: A map where the key is the number of inputs and the entry       *
 *           is the corresponding delay. If there is only one entry at key   *
 *           UNDEFINED, then delay is a constant (doesn't vary with fan-in). *
 *	     A map saves us the trouble of sorting, and has lower access     *
 *           time for interpolation/extrapolation purposes                   *
 * mux_trans_size:  The area of each transistor in the segment's driving mux *
 *                  measured in minimum width transistor units               *
 * buf_size:  The area of the buffer. If set to zero, area should be         *
 *            calculated from R                                              */
typedef struct s_arch_switch_inf {
	bool buffered;
	float R;
	float Cin;
	float Cout;
	std::map< int, double > Tdel_map;
	float mux_trans_size;
	float buf_size;
	char *name;
	e_power_buffer_type power_buffer_type;
	float power_buffer_size;

	s_arch_switch_inf(){
		buffered = false;
		R = 0;
		Cin = 0;
		Cout = 0;
		mux_trans_size = 0;
		buf_size = 0;
		name = NULL;
		power_buffer_type = POWER_BUFFER_TYPE_UNDEFINED;
		power_buffer_size = 0;
	}
} t_arch_switch_inf;

/* Lists all the important information about an rr switch type.              *
 * The s_rr_switch_inf describes a switch derived from a switch described    *
 * by s_arch_switch_inf. This indirection allows us to vary properties of a  *
 * given switch, such as varying delay with switch fan-in.                   * 
 * buffered:  Does this switch include a buffer?                             *
 * R:  Equivalent resistance of the buffer/switch.                           *
 * Cin:  Input capacitance.                                                  *
 * Cout:  Output capacitance.                                                *
 * Tdel:  Intrinsic delay.  The delay through an unloaded switch is          *
 *        Tdel + R * Cout.                                                   *
 * mux_trans_size:  The area of each transistor in the segment's driving mux *
 *                  measured in minimum width transistor units               *
 * buf_size:  The area of the buffer. If set to zero, area should be         *
 *            calculated from R                                              */
typedef struct s_rr_switch_inf {
	bool buffered;
	float R;
	float Cin;
	float Cout;
	float Tdel;
	float mux_trans_size;
	float buf_size;
	char *name;
	e_power_buffer_type power_buffer_type;
	float power_buffer_size;

	s_rr_switch_inf(){
		buffered = false;
		R = 0;
		Cin = 0;
		Cout = 0;
		Tdel = 0;
		mux_trans_size = 0;
		buf_size = 0;
		name = NULL;
		power_buffer_type = POWER_BUFFER_TYPE_UNDEFINED;
		power_buffer_size = 0;
	}
} t_switch_inf;

/* Lists all the important information about a direct chain connection.     *
 * [0 .. det_routing_arch.num_direct]                                       *
 * name:  Name of this direct chain connection                              *
 * from_pin:  The type of the pin that drives this chain connection         *
 In the format of <block_name>.<pin_name>                      *
 * to_pin:  The type of pin that is driven by this chain connection         *
 In the format of <block_name>.<pin_name>                        *
 * x_offset:  The x offset from the source to the sink of this connection   *
 * y_offset:  The y offset from the source to the sink of this connection   *
 * z_offset:  The z offset from the source to the sink of this connection   *
 * switch_type: The index into the switch list for the switch used by this  *
 *              direct                                                      *
 * line: The line number in the .arch file that specifies this              *
 *       particular placement macro.                                        *
 */
typedef struct s_direct_inf {
	char *name;
	char *from_pin;
	char *to_pin;
	int x_offset;
	int y_offset;
	int z_offset;
	int switch_type;
	int line;
} t_direct_inf;


/* Used to list information about a set of track segments that should connect through a switchblock */
typedef struct s_wireconn_inf{
	std::vector<std::string> from_type;		/* connect from these wire types */
	std::vector<std::string> to_type;		/* to these wire types */
	std::vector<int> from_point;			/* indices of wire points belonging to from_type */
	std::vector<int> to_point;			/* indices of wire points belong to to_type (each 'from_point' connects to every 'to_point' */
} t_wireconn_inf;

/* represents a connection between two sides of a switchblock */
class SB_Side_Connection{
public:
	/* specify the two SB sides that form a connection */
	enum e_side from_side;
	enum e_side to_side;

	void set_sides( enum e_side from, enum e_side to ){
		from_side = from;
		to_side = to;
	}

	SB_Side_Connection()
        : from_side(TOP)
        , to_side(TOP) {
		/* do nothing */
	}

	SB_Side_Connection(enum e_side from, enum e_side to)
        : from_side(from)
        , to_side(to) {
	}

	/* overload < operator which will be used by std::map */	
	bool operator < (const SB_Side_Connection &obj) const{
		bool result;

		if (from_side < obj.from_side){
			result = true;
		} else {
			if (from_side == obj.from_side){
				result = (to_side < obj.to_side) ? true : false;
			} else {
				result = false;
			}
		}

		return result;
	}
};

/* Use a map to index into the string permutation functions used to connect from one side to another */
typedef std::map< SB_Side_Connection, std::vector<std::string> > t_permutation_map;

/* Lists all information about a particular switch block specified in the architecture file */
typedef struct s_switchblock_inf{
	std::string name;			/* the name of this switchblock */
	e_sb_location location;			/* where on the FPGA this switchblock should be built (i.e. perimeter, core, everywhere) */
	e_directionality directionality;	/* the directionality of this switchblock (unidir/bidir) */

	t_permutation_map permutation_map;	/* map holding the permutation functions attributed to this switchblock */

	std::vector<t_wireconn_inf> wireconns;	/* list of wire types/groups this SB will connect */
} t_switchblock_inf;


/*   Detailed routing architecture */
typedef struct s_arch t_arch;
struct s_arch {
	t_chan_width_dist Chans;
	enum e_switch_block_type SBType;
	std::vector<t_switchblock_inf> switchblocks;
	float R_minW_nmos;
	float R_minW_pmos;
	int Fs;
	float grid_logic_tile_area;
	struct s_clb_grid clb_grid;
	t_segment_inf * Segments;
	int num_segments;
	struct s_arch_switch_inf *Switches;
	int num_switches;
	t_direct_inf *Directs;
	int num_directs;
	t_model *models;
	t_model *model_library;
	t_power_arch * power;
	t_clock_arch * clocks;
	
	/* Ipin cblock parameters may be set through a switch or through the timing/sizing nodes under <device>.
	   The former sets the ipin_cblock_switch_name. The latter sets the other 3 fields. */
	char *ipin_cblock_switch_name;
	float C_ipin_cblock;
	float T_ipin_cblock;
	float ipin_mux_trans_size;

};

#endif

