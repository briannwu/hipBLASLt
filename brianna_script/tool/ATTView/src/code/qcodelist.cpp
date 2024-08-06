#include <vector>
#include <QComboBox>
#include <QLabel>
#include "include/custom_layouts.h"
#include "qcodelist.h"
#include "graphics/canvas.h"
#include "data/wavemanager.h"
#include <unordered_set>

// Copied from rocmtools. Todo: Link to rocmtools version on merge
/* The function extracts the kernel name from
input string. By using the iterators it finds the
window in the string which contains only the kernel name.
For example 'Foo<int, float>::foo(a[], int (int))' -> 'foo'*/
std::string truncate_name(const std::string& name) {
	auto rit = name.rbegin();
	auto rend = name.rend();
	uint32_t counter = 0;
	char open_token = 0;
	char close_token = 0;
	while (rit != rend) {
		if (counter == 0) {
			switch (*rit) {
		case ')':
			counter = 1;
			open_token = ')';
			close_token = '(';
			break;
		case '>':
			counter = 1;
			open_token = '>';
			close_token = '<';
			break;
		case ']':
			counter = 1;
			open_token = ']';
			close_token = '[';
			break;
		case ' ':
			++rit;
			continue;
		}
		if (counter == 0) break;
		} else {
			if (*rit == open_token) counter++;
			if (*rit == close_token) counter--;
		}
		++rit;
	}
	auto rbeg = rit;
	while ((rit != rend) && (*rit != ' ') && (*rit != ':')) rit++;
	return name.substr(rend - rit, rit - rbeg);
}


QCodeSection::QCodeSection(QWidget *parent): QWidget(parent) { layout = new QVBox(this); }
QCodeSection::~QCodeSection() { delete layout; }
void QCodeSection::AddLine(QCodeline* line) {
	layout->addWidget(line);
	layout->setSpacing(0);
	layout->setMargin(0);
	num_lines ++;
}

QCodelist::QCodelist(QWidget *parent): QWidget(parent) { Reset(); }

QCodeline* QCodelist::AddLine(
	int wave_id,
	int index,
	const std::string& line,
	int asm_line_num,
	const std::vector<int>& cycles,
	const std::string& cppline,
	int64_t cycles_sum,
	int hitcount
)
{
	while (sections.size()*max_section_size <= index) {
		sections.push_back(new QCodeSection(this));
		layout_main->addWidget(sections.back());
	}

	QCodeSection* section = sections.back();
	QCodeline* codeline = new QCodeline(this, wave_id, index, line, asm_line_num, cycles, cppline, cycles_sum, hitcount);
	section->AddLine(codeline);
	lines[index] = {section, codeline};

	//if (line.substr(0,5) == ".Ltmp") codeline->setVisible(false);
	return codeline;
}

QCodelist::~QCodelist() {
	if (layout_main) delete layout_main;
	if (connector) delete connector;
}
/*
class CycleModeSelector: public QComboBox {
    set_tracked();
public:
	CycleModeSelector(class QWidget* parent): QComboBox(parent) {
		this->addItem(QString("Sum all"));
		this->addItem(QString("Mean all"));
		this->addItem(QString("Sum"));
		this->addItem(QString("Mean"));
		this->addItem(QString("Max"));
		this->addItem(QString("First"));
		this->addItem(QString("Last"));
	}
}; */


class CycleModeSelector: public QWidget {
    set_tracked();
public:
	CycleModeSelector(class QWidget* parent) {
		label = new QLabel("Hitcount    Cycles:", parent);
		box = new QComboBox(parent);
		box->addItem(QString("Sum all"));
		box->addItem(QString("Mean all"));
		box->addItem(QString("Sum"));
		box->addItem(QString("Mean"));
		box->addItem(QString("Max"));
		box->addItem(QString("First"));
		box->addItem(QString("Last"));

		parent->layout()->addWidget(label);
		parent->layout()->addWidget(box);
		parent->layout()->setContentsMargins(0, 0, 0, 0);
	}

	QLabel* label;
	QComboBox* box;
};

class FirstLine: public QCodeline {
public:
	FirstLine(QCodelist *parent): QCodeline(parent, -1, 0, "Instruction", -1, {}, "Source Code", -1, -1) {
		this->line->functional_element = new QLabel("Instruction", this->line);
		this->cppline->functional_element = new QLabel("Source Code", this->cppline);
		//this->hitcount->functional_element = new QLabel("Hitcount", this->hitcount);

		CreateLayout();
		this->cycles->setLayout(new QHBox());
		cycles_selector = new CycleModeSelector(this->cycles);
		//this->cycles->functional_element = func_cycles;

		QObject::connect(cycles_selector->box, &QComboBox::currentTextChanged, parent, &QCodelist::changeStrategy);
		QASSERT(QCodeline::firstline == nullptr, "Singleton class not unique!");
		QCodeline::firstline = this;
	}

    virtual void paintEvent(class QPaintEvent* event) override { this->QWidget::paintEvent(event); };
    virtual void mouseMoveEvent(class QMouseEvent* event) override { this->QWidget::mouseMoveEvent(event); };
	virtual QSize sizeHint() const override { return QSize(this->QWidget::sizeHint().width(),LINE_HEIGHT*2); }
	virtual QSize minimumSizeHint() const override { return QSize(this->QWidget::sizeHint().width(),LINE_HEIGHT*2); }

	~FirstLine() {
		QASSERT(QCodeline::firstline != nullptr, "Singleton class deleted twice!");
		QCodeline::firstline = nullptr;

		if (cycles_selector) delete cycles_selector;
	}
	class CycleModeSelector* cycles_selector = nullptr;
    class QHBox* hlayout;
};

void QCodelist::Reset() {
	bInitComplete = false;
	sections = std::vector<class QCodeSection*>();

	if (layout_main) delete layout_main;
	if (connector) delete connector;

	layout_main = new QVBox(this);
	layout_main->setSpacing(0);
	layout_main->setMargin(0);

	connector = new ArrowCanvas();
	lines = std::unordered_map<int, std::pair<class QCodeSection*, class QCodeline*>>();
	QCppLabel::ResetCppFile();
}

void QCodelist::Populate(const std::string& path) {
	Reset();
	WaveManager manager(path);

	waitcnt = std::vector<WaitList>(manager.GetWaitcnt());
	connector->buildConnections(waitcnt);

	//int last_distance = manager.GetCode().size();
	//std::vector<std::pair<int, int>> distance_to_hit{{0, last_distance}};

	layout_main->addWidget(new FirstLine(this));
	for(auto& line : manager.GetCode()) {
		int lwid = (line.cycles.size() && line.clock.size()) ? line.wave_id : -1;
		AddLine(lwid, line.index+1, line.inst, line.asm_line_num, line.cycles, line.cppline, line.hitcount, line.cycles_sum);

		/*last_distance += 1;
		if (line.hitcount)
			last_distance = 0;
		distance_to_hit.push_back({line.index+1, last_distance}); */
	}

	/*for (int i=int(distance_to_hit.size())-1; i>=0; i--) {
		auto& elem = distance_to_hit[i];
		last_distance = std::min(last_distance+1, elem.second);
		elem.second = last_distance;
	}

	std::unordered_set<QCodeSection*> visible_sec;
	if (sections.size()) visible_sec.insert(sections[0]);

	for (int i=0; i<(int)distance_to_hit.size(); i++) {
		auto& elem = distance_to_hit[i];
		if (elem.second < 2)
			visible_sec.insert(lines[elem.first].first);
	}
	for (int i=2; i<(int)distance_to_hit.size(); i++) {
		auto& elem = distance_to_hit[i];
		std::pair<QCodeSection*, QCodeline*>& line = lines[elem.first];
		if (!line.first || !line.second) continue;

		if (visible_sec.find(line.first) == visible_sec.end()) {
			line.first->setVisible(false);
		} else if (elem.second > 5) {
			line.second->setVisible(false);
			line.first->num_lines --;
		}
	} */

	if (sections.size())
		sections.back()->layout->insertStretch(sections.back()->num_lines+1);

	bInitComplete = true;
}

void QCodelist::paintEvent(QPaintEvent* event) {
	this->QWidget::paintEvent(event);

	if (!bInitComplete)
		return;

	QWARNING(connector, "connector missing", return);
	connector->paintCanvas(this);
}

QSize QCodelist::GetSlotSize() {
	if (lines[1].second)
		return lines[1].second->aslot->size();
	for (auto it = lines.begin(); it != lines.end(); it++)
		return it->second.second->aslot->size();
	return QSize(0, 0);
}

QPoint QCodelist::GetLinePos(int lineid) {
	auto& pair = lines[lineid+1];
	QWARNING(pair.first && pair.second, "Invalid line number", return QPoint(-1, -1));

	return 	QPoint( pair.second->pos().x() + pair.first->pos().x(),
					pair.second->pos().y() + pair.first->pos().y());
}

void QCodelist::changeStrategy(const QString& text) {
	for (auto& [_, p] : lines)
	if (p.second && p.second->cycles) {
		p.second->cycles->setStrategy(text);
		p.second->repaint();
	}
}
