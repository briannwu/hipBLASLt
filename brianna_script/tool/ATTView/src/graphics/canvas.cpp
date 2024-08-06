#include "canvas.h"
#include <QBrush>
#include <QPainter>
#include <QPainterPath>
#include "code/qcodelist.h"
#include "data/wavemanager.h"
#include "mainwindow.h"
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>

struct PairHash {
    size_t operator()(const std::pair<int, int>& x) const {
        return (size_t(x.first) << 32) | size_t(x.second);
    }
};

std::vector<QColor> ArrowCanvas::arrow_colors = {
    {0, 150, 255},
    {255, 150, 0},
    {255, 0, 150},
	{128, 0, 255},
    {0, 255, 0},
    {255, 50, 50},
    {50, 50, 255},
    {64, 64, 64},
    {160, 160, 160}
};

void ArrowSlot::mousePressEvent(class QMouseEvent *e) {
    this->Super::mousePressEvent(e);
}

void ArrowCanvas::paintCanvas(class QCodelist* list) {
    QASSERT(list, "QCodelist is null");

    std::vector<class WaitList>& waitcnt = list->waitcnt;

    if (!bInit) // If this is the first paint command, build arrows "cache"
        buildConnections(waitcnt);

    QPainter painter(list);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    colorstate = 0;
    int max_state = 0;
    int last_waitcnt = -1;
    for(auto& arrow : arrows) { // Paint all arrows
        if (std::get<3>(arrow) && last_waitcnt != std::get<0>(arrow)) { // Make interior colors repeat
            max_state = std::max(max_state, colorstate);
            colorstate = 0;
            last_waitcnt = std::get<0>(arrow);
        } else if (!std::get<3>(arrow) && last_waitcnt != -1) {
            last_waitcnt = -1;
            colorstate = max_state; // Restore exterior color to max in interior
        }
        Connect(list, painter, std::get<0>(arrow), std::get<1>(arrow), std::get<2>(arrow));
    }
}

void ArrowCanvas::buildConnections(const std::vector<WaitList>& waitcnt) {
    bInit = true;
    // list of connections jumping over a waitcnt
    std::unordered_set<std::pair<int, int>, PairHash> crossed_connections;
    // list of all connections per waitcnt, key = waitcnt linenumber
    std::map<int, std::set<int>> connections_map;
    // maximum slot used per waitcnt linenumber. Slot defines how far from the edge an arrow is
    //std::map<int, int> slots_map;
    std::map<int, std::unordered_set<int>> slots_map;

    // Make list of all connections
    for (const WaitList& w : waitcnt) {
        if (connections_map.find(w.code_line) == connections_map.end()) {
            connections_map[w.code_line] = std::set<int>({w.code_line});
            slots_map[w.code_line] = std::unordered_set<int>();
        }

        std::set<int>& map = connections_map[w.code_line];

        for (auto& [source, _] : w.sources) {
            map.insert(source);
            slots_map[source] = std::unordered_set<int>();
        }
    }

    // Draw connections on which there is no waitcnt in between src (memory op) and dst (waitcnt)
    int prev_wait = -1;
    for (auto iter = connections_map.begin(); iter != connections_map.end(); ++iter) {
        int next_wait = std::next(iter)==connections_map.end() ? INT_MAX : (*std::next(iter)).first;

        const int wait_number = (*iter).first;
        std::set<int>& conn_set = (*iter).second;

        int prev_slot_n = 0;
        for (auto mem_iter = conn_set.find(wait_number); mem_iter != conn_set.begin(); --mem_iter) {
            int mem_line = *std::prev(mem_iter);

            if (mem_line <= prev_wait) {
                crossed_connections.insert({wait_number, mem_line});
            } else {
                arrows.push_back({wait_number, mem_line, prev_slot_n, true});
                prev_slot_n++;
                for (int ml = 1; ml <= prev_slot_n; ml++)
                    slots_map[mem_line].insert(ml);
            }
        }
        int next_slot_n = 0;
        for (auto mem_iter = std::next(conn_set.find(wait_number)); mem_iter != conn_set.end(); ++mem_iter) {
            if (*mem_iter >= next_wait) {
                crossed_connections.insert({wait_number, *mem_iter});
            } else {
                arrows.push_back({wait_number, *mem_iter, next_slot_n, true});
                next_slot_n++;
                for (int ml = 1; ml <= next_slot_n; ml++)
                    slots_map[*mem_iter].insert(ml);
            }
        }

        for (int ml = 1; ml <= std::max(prev_slot_n, next_slot_n); ml++)
            slots_map[wait_number].insert(ml);
        prev_wait = wait_number;
    }

    // Sort leftover connections by source-destination distance
    std::vector<std::pair<int, int>> crossed_vector(crossed_connections.begin(), crossed_connections.end());
    sort(
        crossed_vector.begin(),
        crossed_vector.end(),
        [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
            return std::abs(a.first-a.second) < std::abs(b.first-b.second);
        }
    );

    max_slot_alloc = 0;
    // Draw sorted leftover connections in sequence and increment slot counters on the way
    for (auto& conn : crossed_vector) { // .first = waitcnt linenumber, .second = mem_op linenumber
        int src = std::min(conn.first, conn.second);
        int dst = std::max(conn.first, conn.second);
        
        auto map_iterator = slots_map.find(conn.first);

        std::unordered_set<int> used_slots;
        if (conn.first < conn.second) {
            for (auto it = map_iterator; it != slots_map.end() && (*it).first <= conn.second; ++it)
                for (int elem : (*it).second)
                    used_slots.insert(elem);
        } else {
            auto rev_map = std::make_reverse_iterator(map_iterator);
            for (auto it = std::prev(rev_map); it != slots_map.rend() && (*it).first >= conn.second; ++it)
                for (int elem : (*it).second)
                    used_slots.insert(elem);
        }

        int slot = 1;
        while (used_slots.find(slot) != used_slots.end())
            slot++;
        
        max_slot_alloc = std::max(slot, max_slot_alloc);
        arrows.push_back({conn.first, conn.second, slot, false});

        if (conn.first < conn.second) {
            for (auto it = map_iterator; it != slots_map.end() && (*it).first <= conn.second; ++it)
                (*it).second.insert(slot); // update all slots in path above
        } else {
            (*map_iterator).second.insert(slot);
            auto rev_map = std::make_reverse_iterator(map_iterator);
            for (auto it = rev_map; it != slots_map.rend() && (*it).first >= conn.second; ++it)
                (*it).second.insert(slot); // update all slots in path below
        }
    }

    std::cout << "Max slot: " << max_slot_alloc << std::endl;
    ArrowSlot::SetWidthRequirements(20+4*max_slot_alloc);
};

void ArrowCanvas::Connect(class QCodelist* list, QPainter& painter, int l1, int l2, int xslot) {
    QASSERT(list, "Invalid codelist");

    QSize _size = list->GetSlotSize();
    int width = _size.width() - 5;
    int height = LINE_HEIGHT;//_size.height();
    
    int posy1 = list->GetLinePos(l1).y();
    int posy2 = list->GetLinePos(l2).y();

    if (width < max_slot_alloc*4) return;

    // --- Begin arrow spacing decision
    int arrow_spacing;
    if (8*(2+max_slot_alloc) < width) arrow_spacing = 8;
    else if (6*(2+max_slot_alloc) < width) arrow_spacing = 6;
    else arrow_spacing = 4;
    // --- End
    // --- Begin arrow out of bounds wrapping
    int horz_length = arrow_spacing*(1+xslot);
    if (horz_length >= width-5)
        horz_length = width/3 + horz_length%(2*width/3);
    // --- End

    //int horz_length = width*(1+xslot)/20;
    double xpos = list->GetLinePos(l1).x() + width - horz_length;

    // ymin and ymax are the center y-positions of line1 and line2 widgets, sorted by min/max
    double ymin = std::min(posy1, posy2) + height/2 + 3;
    double ymax = std::max(posy1, posy2) + height/2 - 2;

    QColor& color = arrow_colors[(colorstate++)%arrow_colors.size()]; // pick color and increment
    painter.fillRect(QRectF(xpos, ymin, 3, ymax-ymin), color); // vertical line from line1 to line2
    painter.fillRect(QRectF(xpos, ymin-2, horz_length, 3), color); // horizontal line at ymin
    painter.fillRect(QRectF(xpos, ymax-2, horz_length, 3), color); // horizontal line at ymax
    xpos += horz_length;

    QPainterPath path;
    path.moveTo(xpos+9, ymin); // Draw triangle at ymin
    path.lineTo(xpos, ymin+4);
    path.lineTo(xpos, ymin-4);
    path.lineTo(xpos+9, ymin);
    path.moveTo(xpos+9, ymax); // Draw triangle at ymax
    path.lineTo(xpos, ymax+4);
    path.lineTo(xpos, ymax-4);
    path.lineTo(xpos+9, ymax);

    painter.fillPath(path, QBrush(color));
}; 

int ArrowSlot::min_width = 256;
