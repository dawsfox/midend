#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct ddg_node_s {
	int statement;
	int weight;
} ddg_node_t;

typedef struct edge_s {
	int weight;
	int src;
	int dest;
} edge_t;

typedef struct ddg_s {
	int start;
	int end;
	int num_edges;
	ddg_node_t node_list[32];
	edge_t edge_list[64];
} ddg_t; 

typedef struct meta_var_s {
	char *var;
	char needs_input;
	char is_tmp;
	char is_assigned;
	char is_if;
} meta_var_t;

void print_ddg(ddg_t curr) {
	printf("ddg from %d to %d\n", curr.start, curr.end);
	printf("has nodes:\n");
	for (int i=0; i<curr.end-curr.start+1; i++) {
		printf("%d, ", curr.node_list[i].statement);
	}
	printf("\nwith edges:\n");
	for (int i=0; i < curr.num_edges; i++) {
		printf("%d->%d, ", curr.edge_list[i].src, curr.edge_list[i].dest);
	}
}

int is_tmp(char *var) {
	if (var[0] == 't' && var[1] == 'm' && var[2] == 'p') {
		return 1;
	}
	return 0;
}

int retrieve_dest(char *three_address, char *dest) {
	unsigned length = strlen(three_address);
	unsigned tab = 0; int if_flag = 0;
	unsigned left_bound;
	if (three_address[0] == '\t') {
		tab = 1;
	}
	else if (three_address[0] == 'I' && three_address[1] == 'f') {
		if_flag = 1;
	}
	for (int i=0; i<length; i++) {
		if (three_address[i] == ' ') {
			strncpy(dest, three_address+tab, i+1);
			dest[i-tab] = '\0';
			return 0;
		}
		else if (if_flag && three_address[i] == '(') {
			left_bound = i+1;
		}
		else if (if_flag && three_address[i] == ')') {
			strncpy(dest, three_address+left_bound, i-left_bound);
			dest[i-left_bound] = '\0';
			return 2;
		}
		else if (three_address[i] == '}') { //line is involved in if statement
			if (three_address[i+1] == ' ' && three_address[i+2] == 'e') { //else beginning
				sprintf(dest, "%s", "else");
			}
			else { //end of if statement
				sprintf(dest, "%s", "end");
			}
			return 1;
		}
	}
	return -1;
}

int retrieve_first_src(char *three_address, char *src1) {
	unsigned length = strlen(three_address);
	int left_bound = 0;
	int single_flag = 0;
	for (int i=0; i<length; i++) {
		char curr = three_address[i];
		if (curr == '=') {
			left_bound = i+2;
		}
		else if ((curr == '/') || (curr == '+') || (curr == '-')) { // all typical cases
			strncpy(src1, three_address+left_bound, i-left_bound);
			src1[i-left_bound] = '\0';
			return i+1;
		}
		else if (curr == '*') {
			strncpy(src1, three_address+left_bound, i-left_bound);
			src1[i-left_bound] = '\0';
			if (three_address[i+1] == '*') { //if pow
				return(i+2);
			}
			else { //just multiplication
				return i+1;
			}
		}
		else if (curr == '!') {
			left_bound = i+1;
		}
		else if (curr == '\n') {
			strncpy(src1, three_address+left_bound, i - left_bound);
			src1[i-left_bound] = '\0';
			return -1; //only one source for the statement
			
		} 
	}
	return -2; //error or unexpected input
}

void retrieve_second_src(char *three_address, char *src2, int left_bound) {
	unsigned length = strlen(three_address);
	for (int i=left_bound; i<length; i++) {
		if (three_address[i] == '\n') {
			three_address[i] = '\0';
			break;
		}
	}
	sprintf(src2, "%s", three_address+left_bound);
}

int is_number(char *src) {
	int i=0;
	char curr = src[0];
	while(isdigit(curr)) {
		i++;
		curr = src[i];
		if (curr == '\0') {
			return 1;
		}
	}
	return 0;
}

int dependency_check(char *var, char **statement, int write) {
	// write is 1 when the given var is a destination, 0 otherwise
	// need NULL protection, otherwise probably seg fault
	if (var != NULL) {
		int is_var = statement[0] != NULL;
		if (is_var && (strcmp(var, statement[0]) == 0)) {
			return 1;
		}
		is_var = statement[1] != NULL;
		if (write && is_var && (strcmp(var, statement[1]) == 0)) {
			return 1;
		}
		is_var = statement[2] != NULL;
		if (write && is_var && (strcmp(var, statement[2]) == 0)) {
			return 1;
		}
	}
	return 0;
}

/*
int node_weight(ddg_node_t *node) {
	return node->weight;
}
*/

int get_latency(char *line) {
	unsigned length = strlen(line);
	int lat = 0;
	for (int i=0; i<length; i++) {
		if (line[i] == '/') {
			lat += 4;
		}
		else if (line[i] == '*') {
			if (line[i+1] == '*') {
				lat += 8;
				i++; //skip '*' on next run through to not double count
			}
			else {
				lat += 4;
			}
		}
		else if (line[i] == '+' || line[i] == '-') {
			lat += 1;
		}
		else if (line[i] == '=') {
			lat += 2;
		}
		else if (line[i] == '\n') {
			return lat;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {

	FILE *input = fopen(argv[1], "r");
	FILE *output = fopen(argv[2], "w");
	int latencies[32];
	char str[80];
	char dest[16];
	char src1[16];
	char src2[16];
	int i=0;
	char **statement_list[32];
	char *whole_statement[32];
	/* while loop parses vars of each line and puts them in appropiate spot in statement_list */
	while (fgets(str, 80, input) != NULL) {
		whole_statement[i] = (char *) malloc(strlen(str) + 1);
		strcpy(whole_statement[i], str);
		latencies[i] = get_latency(str);
		statement_list[i] = (char **) malloc(sizeof(char *) * 3); //allocate space for dest, src1, src2 for this statement
		int result = retrieve_dest(str, dest);
		if(result == 0) {
			statement_list[i][0] = (char *) malloc(sizeof(char) * strlen(dest) + 1); // allocate space for dest including null char
			strcpy(statement_list[i][0], dest);
			printf("statement %d has dest: %s, ", i, statement_list[i][0]);
			result = retrieve_first_src(str, src1);
			if (result == -1) {
				if (!is_number(src1)) {
					statement_list[i][1] = (char *) malloc(sizeof(char) * strlen(src1) + 1);
					strcpy(statement_list[i][1], src1);
					printf("src1: %s, no second source\n", statement_list[i][1]);
				}
				else {
					statement_list[i][1] = NULL;
				}
				statement_list[i][2] = NULL;
			}
			else {
				if(!is_number(src1)) {
					statement_list[i][1] = (char *) malloc(sizeof(char) * strlen(src1) + 1);
					strcpy(statement_list[i][1], src1);
					printf("src1: %s, ", statement_list[i][1]);
				}
				else {
					statement_list[i][1] = NULL;
				}
				retrieve_second_src(str, src2, result);
				if(!is_number(src2)) {
					statement_list[i][2] = (char *) malloc(sizeof(char) * strlen(src2) + 1);
					strcpy(statement_list[i][2], src2);
					printf("src2: %s", statement_list[i][2]);
				}
				else {
					statement_list[i][2] = NULL;
				}
				printf(" (%d%d%d)\n", is_number(dest), is_number(src1), is_number(src2));
			}
		}
		else if (result == 1) {
			statement_list[i][0] = (char *) malloc(sizeof(char) * strlen(dest) + 1); // allocate space for dest including null char
			printf("statement %d is if-statement overhead\n", i);
			strcpy(statement_list[i][0], dest);
			statement_list[i][1] = NULL;
			statement_list[i][2] = NULL;
		}
		else if (result == 2) { //note in this case dest is actually src1
			statement_list[i][0] = NULL;
			statement_list[i][1] = (char *) malloc(sizeof(char) * strlen(dest) + 1); // allocate space for dest including null char
			statement_list[i][2] = NULL;
			strcpy(statement_list[i][1], dest);
			printf("statement %d is if-statement reading %s\n", i, statement_list[i][1]);
		}
		else {
			printf("error retrieving dest\n");
		}
		i++;
	}
	
	/* building ddgs */
	ddg_t blocks[8];
	unsigned block_index = 0;;
	blocks[0].start = 0;
	int if_begin = 0;
	int else_begin = 0;
	int else_end = 0;
	unsigned edge_index = 0;
	unsigned node_index = 0;
	// find endpoint of first basic block
	for (int k=0; k<i; k++) {
		char *dest_curr = statement_list[k][0];
		char *src1_curr = statement_list[k][1];
		char *src2_curr = statement_list[k][2];
		if (dest_curr == NULL && src1_curr != NULL && src2_curr == NULL) {
			blocks[0].end = k-1;
		}
	}
	// loop through to build edges and nodes
	for (int j=0; j<i; j++) {
		//ddg_node_t *curr = malloc(sizeof(ddg_node_t));
		blocks[block_index].node_list[node_index].statement = j;
		blocks[block_index].node_list[node_index].weight = 1;
		char *dest_curr = statement_list[j][0];
		char *src1_curr = statement_list[j][1];
		char *src2_curr = statement_list[j][2];
		// if current block is done
		if (j == blocks[block_index].end+1) {
			blocks[block_index].num_edges = edge_index;
			print_ddg(blocks[block_index]);
			block_index++;
			// find starting point of next block
			for (int k=j+1; k<i; k++) {
				if (statement_list[k][1] == NULL && statement_list[k][2] == NULL) { //either else begin or else end
					if (strcmp(statement_list[k][0], "end") == 0) {
						else_end = k;
						break;
					}
				}
			}
			blocks[block_index].start = else_end+1;
			printf("next block starts at %d\n", blocks[block_index].start);
			// jump j to new starting point
			j = else_end+1;
			// find new blocks end point
			blocks[block_index]. end = i-1; //end of program if no more control flow changes
			for (int k=j; k<i; k++) {
				if (statement_list[k][0] == NULL && statement_list[k][1] != NULL && statement_list[k][2] == NULL) { //next if start
					blocks[block_index].end = k - 1;
				}
			}
			dest_curr = statement_list[j][0];
			src1_curr = statement_list[j][1];
			src2_curr = statement_list[j][2];
			node_index = 0;
			edge_index = 0;
			blocks[block_index].node_list[node_index].statement = j;
			blocks[block_index].node_list[node_index].weight = 1;
		}
		int edge_made = 0;
		for (int k=j+1; k<=blocks[block_index].end; k++) {
			int deps = dependency_check(dest_curr, statement_list[k], 1);
			deps += dependency_check(src1_curr, statement_list[k], 0);
			deps += dependency_check(src2_curr, statement_list[k], 0);
			if (deps > 0) {
				blocks[block_index].edge_list[edge_index].src = j; 
				blocks[block_index].edge_list[edge_index].dest = k;
				blocks[block_index].edge_list[edge_index].weight = latencies[j];
				printf("edge between statement %d and statement %d with weight %d\n", j, k, latencies[j]);
				edge_made = 1;
				edge_index++;
			}
		}
		if (!edge_made) {
				blocks[block_index].edge_list[edge_index].src = j;
				blocks[block_index].edge_list[edge_index].dest = -1; //-1 signifies END
				blocks[block_index].edge_list[edge_index].weight = latencies[j];
				printf("edge between statement %d and END with weight %d\n", j, latencies[j]);
				edge_index++;

		}
		node_index++;
	} //outermost for
	blocks[block_index].num_edges = edge_index;
	print_ddg(blocks[block_index]);

	// build structs to help discern variables to be declared, input, output in C code output
	meta_var_t var_list[32];
	int var_index = 0;
	for (int j=0; j<i; j++) {
		if (statement_list[j][1] != NULL) {
			int k;
			for (k=0; k<var_index; k++) {
				if(var_list[k].var != NULL && strcmp(statement_list[j][1], var_list[k].var) == 0) { //already in list
					if (var_list[k].is_assigned != 1) { //used as src without assignment
						var_list[k].needs_input = 1;
					}
					break;
				}
			}
			if (k == var_index) { // no match found
				var_list[var_index].var = malloc(strlen(statement_list[j][1]) + 1);
				strcpy(var_list[var_index].var, statement_list[j][1]); //add to list
				if (is_tmp(var_list[var_index].var)) {
					var_list[var_index].is_tmp = 1;
				}
				var_list[var_index].needs_input = 1; //src first, so needs input
				var_index++;
			}
		} 
		if (statement_list[j][2] != NULL) {
			int k;
			for (k=0; k<var_index; k++) {
				if(var_list[k].var != NULL && strcmp(statement_list[j][2], var_list[k].var) == 0) { //already in list
					if (var_list[k].is_assigned != 1) { //used as src without assignment
						var_list[k].needs_input = 1;
					}
					break;
				}
			}
			if (k == var_index) { // no match found
				var_list[var_index].var = malloc(strlen(statement_list[j][2]) + 1);
				strcpy(var_list[var_index].var, statement_list[j][2]); //add to list
				if (is_tmp(var_list[var_index].var)) {
					var_list[var_index].is_tmp = 1;
				}
				var_list[var_index].needs_input = 1; //src first, so needs input
				var_index++;
			}
		} 
		if (statement_list[j][0] != NULL) {
			int k;
			for (k=0; k<var_index; k++) { //see if its already been added
				if(var_list[k].var != NULL && strcmp(statement_list[j][0], var_list[k].var) == 0) { //already in var_list
					break;
				}
			}
			if (k == var_index && strcmp(statement_list[j][0],"else") != 0 && strcmp(statement_list[j][0],"end") != 0) { // no match found
				var_list[var_index].var = malloc(strlen(statement_list[j][0]) + 1);
				strcpy(var_list[var_index].var, statement_list[j][0]); //add to list
				if (is_tmp(var_list[var_index].var)) {
					var_list[var_index].is_tmp = 1;
				}
				var_list[var_index].is_assigned = 1;
				var_index++;
			}
		}
	}

	fprintf(output, "main(){\n");
	fprintf(output, "\tint ");
	for (int j=0; j<var_index; j++) {
		if (j != var_index - 1) {
			fprintf(output, "%s, ", var_list[j].var);
		}
		else {
			fprintf(output, "%s;\n\n", var_list[j].var);
		}
	}

	for (int j=0; j<var_index; j++) {
		if (var_list[j].needs_input == 1) {
			fprintf(output, "\tprintf(\"%s=\");\n", var_list[j].var);
			fprintf(output, "\tscanf(\"%%d\",&%s);\n\n", var_list[j].var);
		}
	}
	
	int statement_index = 0; //for statement labeling

	// employ algorithms on each block
	for (int j=0; j<block_index+1; j++) {
		ddg_t curr = blocks[j];
		int est[32]; //earliest starting time
		//for each node
		int k;
		for (k=0; k<curr.end-curr.start+1; k++) {
			ddg_node_t curr_node = curr.node_list[k];
			int est_curr = 0;
			est[k] = 0;
			// for each edge
			for (int l=0; l<curr.num_edges; l++) {
				edge_t curr_edge = curr.edge_list[l];
				if (curr_edge.dest == curr_node.statement) {
					int m;
					for (m=0; m<k; m++) {
						if (curr.node_list[m].statement == curr_edge.src) {
							break;
						}
					}
					int new_est = est[m] + curr.node_list[m].weight + curr_edge.weight;
					printf ("edge comprises est %d, node weight %d, edge weight %d\n", est[m], curr.node_list[m].weight, curr_edge.weight);
					if (new_est > est[k]) {
						printf("max est found for node %d based on statement %d\n", curr_node.statement, curr.node_list[m].statement);
						est[k] = new_est;
					}
				}
			}
			printf("statement %d has EST %d\n", curr_node.statement, est[k]);
		}
		// calculate end
		est[k] = 0;
		for (int l=0; l<curr.num_edges; l++) {
			edge_t curr_edge = curr.edge_list[l];
			if (curr_edge.dest == -1) { // -1 is end
				int m;
				for (m=0; m<k; m++) {
					if (curr.node_list[m].statement == curr_edge.src) {
						break;
					}
				}
				int new_est = est[m] + curr.node_list[m].weight + curr_edge.weight;
				printf ("edge comprises est %d, node weight %d, edge weight %d\n", est[m], curr.node_list[m].weight, curr_edge.weight);
				if (new_est > est[k]) {
					printf("max est found for end based on statement %d\n", curr.node_list[m].statement);
					est[k] = new_est;
				}
			}
		}
		printf("END has EST %d\n", est[k]);
		int end_est = est[k];
		int lst[32]; //latest starting time
		for (int z=0; z<32; z++) {
			lst[z] = end_est;
		}
		for (int l=0; l<curr.num_edges; l++) { //first set compute LST for nodes that have edges to END
			edge_t curr_edge = curr.edge_list[l];
			if (curr_edge.dest == -1) {
				int m;
				for (m=0; m<k; m++) {
					if (curr.node_list[m].statement == curr_edge.src) {
						break;
					}
				}
				int new_lst = end_est - curr_edge.weight - curr.node_list[m].weight;
				printf("min lst (%d) found for statement %d based on END\n", new_lst, curr.node_list[m].statement);
				lst[m] = new_lst;
				//if (new_lst < lst[m]) {
				//}
			}
		}
		int num_nodes = k;
		for (; k >= 0; k--) { //compute rest of nodes' LST
			ddg_node_t curr_node = curr.node_list[k]; //should be END
			for (int l=curr.num_edges-1; l>=0; l--) {
				edge_t curr_edge = curr.edge_list[l];
				if (curr_edge.src == curr_node.statement && curr_edge.dest != -1) {
					int m;
					for (m=0; m<num_nodes; m++) {
						if (curr.node_list[m].statement == curr_edge.dest) {
							break;
						}
					}
					int new_lst = lst[m] - curr_edge.weight - curr.node_list[m].weight;
					printf("edge comprises lst %d, node weight %d, edge weight %d\n", lst[m], curr.node_list[m].weight, curr_edge.weight);
					if (new_lst < lst[k]) {
						printf("min lst (%d) found for node %d based on statement %d\n", new_lst, curr_node.statement, curr.node_list[m].statement);
						lst[k] = new_lst;
					}
				}
			}
		}
		
		for (int l=0; l<num_nodes; l++) {
			printf("statement %d has rank %d\n", curr.node_list[l].statement, lst[l] - est[l]);
		}
		// build priority list, each int is number of statement next in line, organized by rank
		int priority[32];
		printf("priority list: ");
		int n;
		int priority_index = 0;
		for (int m=0; m<end_est; m++) {
			for (n=0; n<num_nodes; n++) {
				if (lst[n] - est[n] == m) { //earliest ranks first
					priority[priority_index] = curr.node_list[n].statement;
					printf("%d, ", priority[priority_index]);
					priority_index++;
				}
			}
		}
		printf("\n");
		// build array of pred_count
		int pred_count[32];
		for (int l=0; l<32; l++) {
			pred_count[l] = 0; //initialize to zero
		}
		for (int l=0; l<curr.num_edges; l++) {
			edge_t curr_edge = curr.edge_list[l];
			if (curr_edge.dest != -1) {
				printf("pred_count using edge from %d to %d\n", curr_edge.src, curr_edge.dest);
				pred_count[curr_edge.dest] += 1; //add 1 for each edge to the node
			}
		}
		printf("pred_count: ");
		for (int l=0; l<num_nodes; l++) {
			printf("%d, ", pred_count[priority[l]]);
		}
		printf("\n");
		int ready_instructions = 0;
		for (int l=0; l<num_nodes; l++) {
			if (pred_count[priority[l]] == 0) {
				printf("%d starts ready\n", curr.node_list[l].statement);
				ready_instructions++;
			}
		}
		//have pred_count for each instruction
		//instructions are ready when pred_count is zero, lower rank goes first
		//reduce pred_count when a predecessor is scheduled (based on edges)
		int used[4];
		used[0] = -1;
		used[1] = -1;
		used[2] = -1;
		used[3] = -1;
		int used_index = 0;
		while (ready_instructions > 0) {
			for (int l=0; l<num_nodes; l++) {
				if (pred_count[priority[l]] == 0) {
					fprintf(output, "\tS%d:\t%s", statement_index, whole_statement[priority[l]]);
					statement_index++;
					printf("scheduled %d \n", priority[l]);
					pred_count[priority[l]] = -1; // pred_count goes to negative one when output
					ready_instructions--;
					int n=0;
					while (n<latencies[priority[l]] && ready_instructions > 0) {
						for (int p=0; p<num_nodes; p++) {
							if (pred_count[priority[p]] == 0 && used_index <= 3) {
								fprintf(output, "\tS%d:\t%s", statement_index, whole_statement[priority[p]]);
								used[used_index] = priority[p];
								used_index++;
								statement_index++;
								printf("scheduled %d \n", priority[p]);
								pred_count[priority[p]] = -1; // pred_count goes to negative one when output
								ready_instructions--;
							}
						}
						n++;
					}
					for (int m=0; m<curr.num_edges; m++) {
						edge_t curr_edge = curr.edge_list[m];
						if (curr_edge.src == priority[l] && curr_edge.dest != -1) {
							printf("%d scheduled, decrement %d's pred_count\n", priority[l], curr_edge.dest);
							pred_count[curr_edge.dest] -= 1;
						}
						else if (curr_edge.src == used[0] && curr_edge.dest != -1) {
							printf("%d scheduled, decrement %d's pred_count\n", used[0], curr_edge.dest);
							pred_count[curr_edge.dest] -= 1;
						}
						else if (curr_edge.src == used[1] && curr_edge.dest != -1) {
							printf("%d scheduled, decrement %d's pred_count\n", used[1], curr_edge.dest);
							pred_count[curr_edge.dest] -= 1;
						}
						else if (curr_edge.src == used[2] && curr_edge.dest != -1) {
							printf("%d scheduled, decrement %d's pred_count\n", used[2], curr_edge.dest);
							pred_count[curr_edge.dest] -= 1;
						}
						else if (curr_edge.src == used[3] && curr_edge.dest != -1) {
							printf("%d scheduled, decrement %d's pred_count\n", used[3], curr_edge.dest);
							pred_count[curr_edge.dest] -= 1;
						}
					}
					used_index = 0;
					used[0] = -1;
					used[1] = -1;
					used[2] = -1;
					used[3] = -1;
					break; //break loop
				}
			}
			ready_instructions = 0;
			for (int l=0; l<num_nodes; l++) {
				if (pred_count[curr.node_list[l].statement] == 0) {
					printf("%d is now ready\n", curr.node_list[l].statement);
					ready_instructions++;
				}
			}
		} 
		if (j < block_index && curr.end < blocks[j+1].start - 1) { //if gap between basic blocks (if statement)
			for (int l=curr.end+1; l<blocks[j+1].start; l++) {
				fprintf(output, "\tS%d:\t%s", statement_index, whole_statement[l]);
				statement_index++;
			}
		}
	}

	fprintf(output, "\n");
	for (int j=0; j<var_index; j++) {
		if (var_list[j].is_tmp != 1) {
			fprintf(output, "\tprintln(\"%s=%%d\\n\");\n", var_list[j].var);
		}
	}
	fprintf(output, "}");
	
	
	
	/* freeing memory used by statement_list */
	for (int j=i-1; j>=0; j--) {
		for (int k=0; k<3; k++) {
			if (statement_list[j][k] != NULL) {
				free(statement_list[j][k]);
			}
		}
		free(statement_list[j]);
	}
	fclose(input);
	fclose(output);
}
