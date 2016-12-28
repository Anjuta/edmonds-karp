#include <mpi.h>
#include <fstream>
#include <stdio.h>
#include <string>

using namespace std;


const int inf = 1000*1000*1000;


bool is_mine_vertex(int v, int rank, int chunk_size) {
    int v_rank = v / chunk_size;
    return rank == v_rank;
}

int global_to_local(int global_v, int chunk_size) {
    return global_v % chunk_size;
}

int local_to_global(int local_v, int rank, int chunk_size) {
    return local_v + rank * chunk_size;
}

int get_matrix_value(const int * matrix, int i, int j, int n) {
    return matrix[i * n + j];
}

void update_matrix_value(int * matrix, int diff, int i, int j, int n, int rank) {
    matrix[i * n + j] += diff;
}

int find_min_diff(
        const int *from, const int *c, const int *flow, int n,
        int rank, int chunk_size) {
    int min_diff = inf;
    int i = n - 1;
    while (i != 0) {
        if (is_mine_vertex(from[i], rank, chunk_size)) {
            int local_v = global_to_local(from[i], chunk_size);
            int current_diff = get_matrix_value(c, local_v, i, n) - get_matrix_value(flow, local_v, i, n);
            min_diff = min(min_diff, current_diff);
        }
        i = from[i];
    }
    return min_diff;
}

void bfs_to_sink(
        const int *c_part, const int *flow_part, int n,
        int rank, int chunk_size,
        int *&from) {
    // initilize from array
    fill_n(from, n, -1);
    from[0] = 0;
    
    // create and initialize current level vertexes
    bool * current = new bool[n];
    fill_n(current, n, false);
    current[0] = true;
    
    while (from[n - 1] == -1) {
        // check if current level vertexes exist
        int i;
        for (i = 0; i < n; i++)
            if (current[i])
                break;
        if (i == n)
            return;

        // initialize array for vertexes for next level
        bool * new_current = new bool[n];
        fill_n(new_current, n, false);

        // spread from array to nodes
        MPI_Bcast(from, n, MPI_INT, 0, MPI_COMM_WORLD);
        
        // create and initilize level vertexes for current node
        bool *current_part = new bool[chunk_size];
        MPI_Scatter(current, chunk_size, MPI::BOOL,
                    current_part, chunk_size, MPI::BOOL,
                    0, MPI_COMM_WORLD);

        for (int i = 0; i < chunk_size; i++)
        {

            // todo не оптимально высчитывать v каждый раз.
            // exit if vertex is not from graph
            int v = local_to_global(i, rank, chunk_size);
            if (v >= n){
                break;
            }

            // find next level vertexes
            if (current_part[i]) {
                for (int j = 0; j < n; j++) {
                    if (get_matrix_value(c_part, i, j, n) - get_matrix_value(flow_part, i, j, n) > 0 
                            && from[j] == -1) {
                        from[j] = v;
                        new_current[j] = true;
                    }
                }
            }
        }
        
        // actualize current level vertexes
        MPI_Allreduce(new_current, current, n, MPI::BOOL, MPI_LOR, MPI_COMM_WORLD);

        // todo не оптимально выделять память каждый раз заново
        // actualize from array
        int *new_from = new int[n];
        fill_n(new_from, n, -1);
        MPI_Allreduce(from, new_from, n, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        from = new_from;
    }
}

void extend_flow(int diff, const int *from, int n, int rank, int chunk_size, int *flow_part) {
    int i = n - 1;
    while (i != 0) {
        if (is_mine_vertex(from[i], rank, chunk_size)) {
            update_matrix_value(flow_part, diff, global_to_local(from[i], chunk_size), i, n, rank);
        }
        if (is_mine_vertex(i, rank, chunk_size)) {
            update_matrix_value(flow_part, -diff, global_to_local(i, chunk_size), from[i], n, rank);
        }
        i = from[i];
    }
}


int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);
    
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int n;
    int *c;
    if (world_rank == 0) {
        // Read graph
        ifstream fin;
        string fname(argv[1]);
        fin.open(fname.c_str());
        fin >> n;
        
        int array_size = n + world_size - (n % world_size);
        c = new int[array_size * array_size];
        fill_n(c, 0, array_size * array_size);
        for (int i=0; i<n; i++)
            for (int j=0; j<n; j++)
                fin >> c[i * n + j];
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    int chunk_size = n / world_size + (int)(n % world_size > 0);
    int graph_part_size = chunk_size * n;
    
    // initialize graph parts per each process
    int * graph_part = new int[graph_part_size];
    MPI_Scatter(c, graph_part_size, MPI_INT,
                graph_part, graph_part_size, MPI_INT,
                0, MPI_COMM_WORLD);

    // initialize flow part per each process
    int * flow_part = new int[graph_part_size];
    fill_n(flow_part, graph_part_size, 0);

    while (true) {
        int * from = new int[n];
        bfs_to_sink(
            graph_part, flow_part, n,
            world_rank, chunk_size,
            from
        );
        if (from[n - 1] == -1)
            break;

        int local_min_diff = find_min_diff(from, graph_part, flow_part, n, world_rank, chunk_size);
        int min_diff;
        MPI_Allreduce(&local_min_diff, &min_diff, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        extend_flow(min_diff, from, n, world_rank, chunk_size, flow_part);
    }

    if (world_rank == 0) {
        int total_flow = 0;
        for (int i = 0; i < n; i++)
            total_flow += flow_part[i];


        printf("%d\n", total_flow);
    }

    MPI_Finalize();
}
