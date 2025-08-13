//
// Created by Enno Adler on 08.08.25.
//

#include "modify.hpp"
#include <sdsl/enc_vector.hpp>
#include <sdsl/int_vector.hpp>

using namespace std;
using namespace sdsl;

int modify_delete_edge(CompressedHyperGraph &hgraph, Index pos)
{
    // 1. Compute deleted positions
    bit_vector deletes(hgraph.D.size(), 0);
    Index i_deletes = pos;
    Index length_reduce = 1;

    deletes[pos] = 1;
    while (hgraph.PSI[i_deletes] != pos)
    {
        i_deletes = hgraph.PSI[i_deletes];
        deletes[i_deletes] = 1;
        length_reduce++;
    }

    rank_support_v<1> rank_deletes(&deletes);
    //select_support_mcl<1> select_deletes(&deletes);

    // 2. Compute updated D
    bit_vector new_d(hgraph.D.size() - length_reduce, 0); // +1 for an additional 1 at the end to enable interval search via select commands.
    Index i_new_d = 0;
    for (Index i_old_d = 0; i_old_d < hgraph.D.size(); i_old_d++)
    {
        if (deletes[i_old_d] == 0)
        {
            new_d[i_new_d] = hgraph.D[i_old_d];
            i_new_d++;
        }
        else if (hgraph.D[i_old_d] == 1)
        {
            new_d[i_new_d] = 1;
            i_new_d++;
            i_old_d++;
            if (hgraph.D[i_old_d] == 1) // Check for two consecutive 1s
                throw -1; // Note would be deleted
        }
    }

    // 3. Compute updated PSI
    int_vector<> new_psi(hgraph.PSI.size() - length_reduce);
    Index i_new_psi = 0;
    for (Index i_old_psi = 0; i_old_psi < hgraph.PSI.size(); i_old_psi++)
    {
        if (deletes[i_old_psi] == 0)
        {
            new_psi[i_new_psi] = hgraph.PSI[i_old_psi] - rank_deletes.rank(hgraph.PSI[i_old_psi]);
            i_new_psi++;
        }
    }

    // 4. Override old arrays.
    enc_vector<> comp_psi(new_psi);
    hgraph.D = std::move(new_d);
    hgraph.PSI = std::move(comp_psi);
    return 0;
}

void modify_intervals_downward_sort(int_vector<> &new_psi, bit_vector &new_d, Index pos)
{
    rank_support_v<1> rank_new_d(&new_d);
    select_support_mcl<1> select_new_d(&new_d);
    bool changed = true;
    Index changed_pos = pos;
    // while (new_psi[changed_pos] != pos) // The old position is 1 smaller than the current started.
    //    changed_pos = new_psi[changed_pos];

    while (changed)
    {
        changed = false;
        Node next_node = rank_new_d.rank(changed_pos)-1;
        Index interval_start = select_new_d.select(next_node+1);
        // Index interval_end = select_new_d.select(next_node+2);

        // Is resorting necessary?
        Index switch_position = changed_pos;
        while (switch_position-1 >= interval_start && new_psi[switch_position-1] > new_psi[switch_position])
        {
            switch_position--;
        }
        if (switch_position != changed_pos) //The actual check.
            changed = true;
        else
            break;

        // Resort interval
        Index help = new_psi[changed_pos];
        for (Index new_pos = changed_pos; new_pos > switch_position; new_pos--)
            new_psi[new_pos] = new_psi[new_pos - 1];
        new_psi[switch_position] = help;

        // fix jumps of moved nodes.
        for (Index update_position = switch_position+1; update_position <= changed_pos; update_position++)
        {
            Index edge_help_pos = update_position; // Find the positions that point to the old position.
            while (new_psi[edge_help_pos] != update_position-1) // The old position is 1 smaller than the current started.
                edge_help_pos = new_psi[edge_help_pos];
            new_psi[edge_help_pos]++; // Node is moved just one position up.
        }

        // fix and set the changed edge
        Index edge_help_pos = switch_position; // Find the positions that point to the old position.
        while (new_psi[edge_help_pos] != changed_pos) // The old position is 1 smaller than the current started.
            edge_help_pos = new_psi[edge_help_pos];
        new_psi[edge_help_pos] = switch_position; // Node is moved just one position up.

        changed_pos = edge_help_pos; // define next iterations update position.
    }
}

void modify_intervals_upward_sort(int_vector<> &new_psi, bit_vector &new_d, Index pos)
{
    rank_support_v<1> rank_new_d(&new_d);
    select_support_mcl<1> select_new_d(&new_d);
    bool changed = true;
    Index changed_pos = pos;
    // while (new_psi[changed_pos] != pos) // The old position is 1 smaller than the current started.
    //    changed_pos = new_psi[changed_pos];

    while (changed)
    {
        changed = false;
        Node next_node = rank_new_d.rank(changed_pos)-1;
        // Index interval_start = select_new_d.select(next_node + 1);
        Index interval_end = select_new_d.select(next_node + 2);

        // Is resorting necessary?
        Index switch_position = changed_pos;
        while (switch_position + 1 < interval_end && new_psi[switch_position] > new_psi[switch_position + 1])
        {
            switch_position++;
        }
        if (switch_position != changed_pos) //The actual check.
            changed = true;
        else
            break;

        // Resort interval
        Index help = new_psi[changed_pos];
        for (Index new_pos = changed_pos; new_pos < switch_position; new_pos++)
            new_psi[new_pos] = new_psi[new_pos + 1];
        new_psi[switch_position] = help;

        // fix jumps of moved nodes.
        for (Index update_position = changed_pos; update_position < switch_position; update_position++)
        {
            Index edge_help_pos = update_position; // Find the positions that point to the old position.
            while (new_psi[edge_help_pos] != update_position+1) // The old position is 1 greater than the current started.
                edge_help_pos = new_psi[edge_help_pos];
            new_psi[edge_help_pos]--; // Node is moved just one position down.
        }

        // fix and set the changed edge
        Index edge_help_pos = switch_position; // Find the positions that point to the old position.
        while (new_psi[edge_help_pos] != changed_pos) // The old position is changed_pos.
            edge_help_pos = new_psi[edge_help_pos];
        new_psi[edge_help_pos] = switch_position;

        changed_pos = edge_help_pos; // define next iterations update position.
    }
}

int modify_delete_node_from_edge(CompressedHyperGraph &hgraph, Index pos, Node node)
{
    // 1. Compute deleted positions
    Index i_deletes = pos;
    rank_support_v<1> rank_d(&hgraph.D);
    Index pos_delete = -1;

    while (hgraph.PSI[i_deletes] != pos)
    {
        i_deletes = hgraph.PSI[i_deletes];
        Node current = rank_d.rank(i_deletes)-1;
        if (current == node)
        {
            pos_delete = i_deletes;
            break;
        }
    }
    if (pos_delete == -1)
        return 0; // There is no such node to delete.

    // 2. Compute updated D
    bit_vector new_d(hgraph.D.size() - 1, 0);
    Index i_new_d = 0;
    for (Index i_old_d = 0; i_old_d < hgraph.D.size(); i_old_d++)
    {
        if (i_old_d != pos_delete)
        {
            new_d[i_new_d] = hgraph.D[i_old_d];
            i_new_d++;
        }
        else if (hgraph.D[i_old_d] == 1)
        {
            new_d[i_new_d] = 1;
            i_new_d++;
            i_old_d++;
            if (hgraph.D[i_old_d] == 1)
                throw -1; // Note would be deleted
        }
    }


    // 3. Compute updated PSI
    int_vector<> new_psi(hgraph.PSI.size() - 1);
    Index i_new_psi = 0;
    bool replaced_last_node = false;
    Index pos_jump_changed = -1;
    for (Index i_old_psi = 0; i_old_psi < hgraph.PSI.size(); i_old_psi++)
    {
        if (i_old_psi != pos_delete)
        {
            // In decreasing order of chance of occurrence
            if (hgraph.PSI[i_old_psi] < pos_delete)
                new_psi[i_new_psi] = hgraph.PSI[i_old_psi];
            else if (hgraph.PSI[i_old_psi] > pos_delete)
                new_psi[i_new_psi] = hgraph.PSI[i_old_psi] - 1;
            else if (hgraph.PSI[hgraph.PSI[i_old_psi]] > pos_delete)
            {
                new_psi[i_new_psi] = hgraph.PSI[hgraph.PSI[i_old_psi]] - 1; // Skip Jump
                pos_jump_changed = i_new_psi;
            }
            else
            { // The last node of an edge was deleted -> new L-Type position.
                new_psi[i_new_psi] = hgraph.PSI[hgraph.PSI[i_old_psi]]; // Skip Jump
                replaced_last_node = true;
                pos_jump_changed = i_new_psi;
            }
            i_new_psi++;
        }
    }

    // 4. Fix intervals.
    if (!replaced_last_node)
        modify_intervals_upward_sort(new_psi, new_d, pos_jump_changed);
    else
        modify_intervals_downward_sort(new_psi, new_d, pos_jump_changed);


    // 5. Override old arrays.
    enc_vector<> comp_psi(new_psi);
    hgraph.D = std::move(new_d);
    hgraph.PSI = std::move(comp_psi);
    return 0;
}

Index modify_search_insert_position(enc_vector<> &PSI, Index from, Index to, Index next_position)
{
    // Solution inspired by ChatGPT
    // Finding `low` (smallest index where psi[low] >= next_from)
    auto low_it = std::lower_bound(
            PSI.begin() + from,
            PSI.begin() + to,
            next_position);
    return std::distance(PSI.begin(), low_it);
}

int modify_insert_node_to_edge(CompressedHyperGraph &hgraph, Index pos, Node node)
{
    // 1. Compute Insert-Positions
    rank_support_v<1> rank_d(&hgraph.D);
    select_support_mcl<1> select_d(&hgraph.D);
    Index i_prev = pos;
    Node node_prev;
    Index i_after = hgraph.PSI[i_prev];
    Node node_after = rank_d.rank(i_after)-1;
    while (i_after != pos // should never happen. Aborts if the full cycle is iterated.
        && !(node_prev < node && node < node_after) // the node is in between,
        && !(i_prev > i_after && (node_prev < node || node < node_after))) // the node is greater than all || the node is lower than all
    {
        i_prev = i_after;
        node_prev = node_after;
        i_after = hgraph.PSI[i_after];
        node_after = rank_d.rank(i_after)-1;
        if (node_after == node)
        {
            return 0; // The Node is already in this hyperedge.
        }
    }
    Index insert_position = modify_search_insert_position(
            hgraph.PSI,
            select_d.select(node+1),
            select_d.select(node+2),
            i_after);

    // 2. Compute updated D
    bit_vector new_d(hgraph.D.size() + 1, 0);
    Index i_new_d = 0;
    Node node_current = -1;
    for (Index i_old_d = 0; i_old_d < hgraph.D.size(); i_old_d++)
    {
        new_d[i_new_d] = hgraph.D[i_old_d];
        i_new_d++;
        if (hgraph.D[i_old_d] == 1)
        {
            node_current++;
            if (node_current == node)
            {
                new_d[i_new_d] = false;
                i_new_d++;
            }
        }
    }

    // 3. Compute updated PSI
    int_vector<> new_psi(hgraph.PSI.size() + 1);
    Index i_new_psi = 0;
    for (Index i_old_psi = 0; i_old_psi < hgraph.PSI.size(); i_old_psi++)
    {
        if (i_new_psi == insert_position)
            i_new_psi++;
        // In decreasing order of chance of occurrence
        if (hgraph.PSI[i_old_psi] < insert_position)
            new_psi[i_new_psi] = hgraph.PSI[i_old_psi];
        else if (hgraph.PSI[i_old_psi] >= insert_position)
            new_psi[i_new_psi] = hgraph.PSI[i_old_psi] + 1;
        i_new_psi++;

    }
    new_psi[i_prev] = insert_position;
    new_psi[insert_position] = i_after;


    // 4. Fix intervals. TODO: correct?
    if (new_psi[insert_position] < insert_position) // If new node is the new highest node of the edge.
        modify_intervals_upward_sort(new_psi, new_d, i_prev);
    else
        modify_intervals_downward_sort(new_psi, new_d, i_prev);

    // 5. Override old arrays.
    enc_vector<> comp_psi(new_psi);
    hgraph.D = std::move(new_d);
    hgraph.PSI = std::move(comp_psi);
    return 0;
}



int modify_insert_edge(CompressedHyperGraph &hgraph, Edge edge)
{

}