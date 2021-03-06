/***************************************************************************
 *   Copyright (C) by gempa GmbH                                           *
 *                                                                         *
 *   You can redistribute and/or modify this program under the             *
 *   terms of the SeisComP Public License.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   SeisComP Public License for more details.                             *
 ***************************************************************************/

#include <seiscomp3/gui/core/application.h>
#include <seiscomp3/gui/core/utils.h>
#include <seiscomp3/gui/map/layers/geofeaturelayer.h>
#include <seiscomp3/gui/map/canvas.h>
#include <seiscomp3/gui/map/standardlegend.h>
#include <seiscomp3/geo/geofeatureset.h>
#include <seiscomp3/seismology/regions.h>

#include <iostream>

using namespace std;

namespace Seiscomp {
namespace Gui {
namespace Map {

namespace {


#define CFG_LAYER_PREFIX "map.layers"

QPen readPen(const Config::Config &cfg, const string &query, const QPen &base) {
	QPen p(base);

	try {
		const string &q = query + ".color";
		p.setColor(readColor(q, cfg.getString(q), base.color()));
	}
	catch ( ... ) {}

	try {
		const string &q = query + ".style";
		p.setStyle(readPenStyle(q, cfg.getString(q), base.style()));
	}
	catch ( ... ) {}

	try {
		p.setWidth(cfg.getDouble(query + ".width"));
	}
	catch ( ... ) {}

	return p;
}

QBrush readBrush(const Config::Config &cfg, const string &query, const QBrush &base) {
	QBrush b(base);

	try {
		const string &q = query + ".color";
		b.setColor(readColor(q, cfg.getString(q), base.color()));
	}
	catch ( ... ) {}

	try {
		const string &q = query + ".style";
		b.setStyle(readBrushStyle(q, cfg.getString(q), base.style()));
	}
	catch ( ... ) {}

	return b;
}

QFont readFont(const Config::Config &cfg, const string& query, const QFont &base) {
	QFont f(base);

	try {
		f.setFamily(cfg.getString(query + ".family").c_str());
	}
	catch ( ... ) {}

	try {
		f.setPointSize(cfg.getInt(query + ".size"));
	}
	catch ( ... ) {}

	try {
		f.setBold(cfg.getBool(query + ".bold"));
	}
	catch ( ... ) {}

	try {
		f.setItalic(cfg.getBool(query + ".italic"));
	}
	catch ( ... ) {}

	try {
		f.setUnderline(cfg.getBool(query + ".underline"));
	}
	catch ( ... ) {}

	try {
		f.setOverline(cfg.getBool(query + ".overline"));
	}
	catch ( ... ) {}

	return f;
}

bool compareByIndex(const LayerProperties *p1, const LayerProperties *p2) {
	return p1->index < p2->index;
}

Qt::Orientation getOrientation(const std::string &name) {
	if ( name == "horizontal" )
		return Qt::Horizontal;
	else if ( name == "vertical" )
		return Qt::Vertical;
	else
		return Qt::Vertical;
}

Qt::Alignment getAlignment(const std::string &name) {
	if ( name == "topleft" )
		return Qt::Alignment(Qt::AlignTop | Qt::AlignLeft);
	else if ( name == "topright" )
		return Qt::Alignment(Qt::AlignTop | Qt::AlignRight);
	else if ( name == "bottomleft" )
		return Qt::Alignment(Qt::AlignBottom | Qt::AlignLeft);
	else if ( name == "bottomright" )
		return Qt::Alignment(Qt::AlignBottom | Qt::AlignRight);
	else
		return Qt::Alignment(Qt::AlignTop | Qt::AlignLeft);
}

void readLayerProperties(LayerProperties *props, const string &dataDir = "") {
	const static string cfgVisible = "visible";
	const static string cfgPen = "pen";
	const static string cfgBrush = "brush";
	const static string cfgFont = "font";
	const static string cfgDrawName = "drawName";
	const static string cfgDebug = "debug";
	const static string cfgRank = "rank";
	const static string cfgRoughness = "roughness";
	const static string cfgTitle = "title";
	const static string cfgLabel = "label";
	const static string cfgIndex = "index";
	const static string cfgLegendArea = "legendArea";
	const static string cfgLegendOrientation = "orientation";

	// Read additional configuration file (e.g. map.cfg in BNA folder)
	if ( !dataDir.empty() ) {
		Config::Config cfg;
		if ( cfg.readConfig(dataDir + "/map.cfg", -1, true) ) {
			try { props->visible = cfg.getBool(cfgVisible); } catch( ... ) {}
			props->pen = readPen(cfg, cfgPen, props->pen);
			props->brush = readBrush(cfg, cfgBrush, props->brush);
			props->font = readFont(cfg, cfgFont, props->font);
			try { props->drawName = cfg.getBool(cfgDrawName); } catch( ... ) {}
			try { props->debug = cfg.getBool(cfgDebug); } catch( ... ) {}
			try { props->rank = cfg.getInt(cfgRank); } catch( ... ) {}
			try { props->roughness = cfg.getInt(cfgRoughness); } catch( ... ) {}
			try { props->title = cfg.getString(cfgTitle); } catch( ... ) {}
			try { props->label = cfg.getString(cfgLabel); } catch( ... ) {}
			try { props->index = cfg.getInt(cfgIndex); } catch( ... ) {}
			try { props->orientation = getOrientation(cfg.getString(cfgLegendOrientation)); } catch( ... ) {}
			try { props->legendArea = getAlignment(cfg.getString(cfgLegendArea)); } catch( ... ) {}
		}
	}

	// Query properties from config
	string query = CFG_LAYER_PREFIX ".";
	if ( !props->name.empty() ) query += props->name + ".";

	if ( SCApp ) {
		try { props->visible = SCApp->configGetBool(query + cfgVisible); } catch( ... ) {}
		props->pen = SCApp->configGetPen(query + cfgPen, props->pen);
		props->brush = SCApp->configGetBrush(query + cfgBrush, props->brush);
		props->font = SCApp->configGetFont(query + cfgFont, props->font);
		try { props->drawName = SCApp->configGetBool(query + cfgDrawName); } catch( ... ) {}
		try { props->debug = SCApp->configGetBool(query + cfgDebug); } catch( ... ) {}
		try { props->rank = SCApp->configGetInt(query + cfgRank); } catch( ... ) {}
		try { props->roughness = SCApp->configGetInt(query + cfgRoughness); } catch( ... ) {}
		try { props->title = SCApp->configGetString(query + cfgTitle); } catch( ... ) {}
		try { props->label = SCApp->configGetString(query + cfgLabel); } catch( ... ) {}
		try { props->index = SCApp->configGetInt(query + cfgIndex); } catch( ... ) {}
		try { props->orientation = getOrientation(SCApp->configGetString(query + cfgLegendOrientation)); } catch( ... ) {}
		try { props->legendArea = getAlignment(SCApp->configGetString(query + cfgLegendArea)); } catch( ... ) {}
	}

	props->filled = props->brush.style() != Qt::NoBrush;
}


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GeoFeatureLayer::GeoFeatureLayer(QObject* parent)
: Layer(parent)
, _initialized(false) {
	setName("features");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GeoFeatureLayer::~GeoFeatureLayer() {
	for ( size_t i = 0; i < _layerProperties.size(); ++i )
		delete _layerProperties[i];
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GeoFeatureLayer::setVisible(bool flag) {
	if ( flag == isVisible() ) return;
	Layer::setVisible(flag);
	emit updateRequested(RasterLayer);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GeoFeatureLayer::bufferUpdated(Canvas *canvas) {
	if ( !_initialized ) {
		_initialized = true;
		initLayerProperites();
	}

	const Geo::GeoFeatureSet &featureSet = Geo::GeoFeatureSetSingleton::getInstance();

	size_t linesPlotted = 0;
	size_t polygonsPlotted = 0;

	size_t categoryId = 0;
	LayerProperties* layProp = NULL;

	// Debug pen and label point
	QPen debugPen;
	debugPen.setColor(Qt::black);
	debugPen.setWidth(1);
	debugPen.setStyle(Qt::SolidLine);

	bool filled = false;
	int zoomLevel = canvas->zoomLevel();

	QPainter painter(&canvas->buffer());
	painter.setRenderHint(QPainter::Antialiasing,
	                      !canvas->previewMode() && SCScheme.map.vectorLayerAntiAlias);

	// Iterate over all features
	vector<Geo::GeoFeature*>::const_iterator itf = featureSet.features().begin();
	for ( ; itf != featureSet.features().end(); ++itf ) {
		// Update painter settings if necessary
		if ( layProp == NULL || categoryId != (*itf)->category()->id ) {
			categoryId = (*itf)->category()->id;
			layProp = _layerProperties.at(categoryId);
			filled = canvas->projection()->isRectangular()?layProp->filled:false;
			painter.setFont(layProp->font);
			painter.setPen(layProp->pen);
			if ( filled ) painter.setBrush(layProp->brush);
		}

		if ( !canvas->drawGeoFeature(painter, *itf, layProp, debugPen, linesPlotted,
		                             polygonsPlotted, filled) ) break;
	}

	// Last property is for "fep"
	const Geo::PolyRegions &fepRegions = Regions().polyRegions();
	layProp = fepRegions.regionCount() > 0 ? _layerProperties.back() : NULL;

	// Skip, if the layer was disabled
	if ( layProp && layProp->visible && layProp->rank <= zoomLevel ) {
		painter.setFont(layProp->font);
		painter.setPen(layProp->pen);
		filled = canvas->projection()->isRectangular()?layProp->filled:false;
		if ( filled ) painter.setBrush(layProp->brush);
		for ( size_t i = 0; i < fepRegions.regionCount(); ++i ) {
			Geo::GeoFeature *reg = fepRegions.region(i);
			if ( !canvas->drawGeoFeature(painter, reg, layProp, debugPen, linesPlotted,
			                             polygonsPlotted, filled) ) break;
		}
	}

	/*
	if ( polygonsPlotted > 0 ) {
		SEISCOMP_DEBUG("zoom: %f, pixelPerDegree: %f -- %i polygons with %i lines plotted",
		               _zoomLevel, _projection->pixelPerDegree(),
		               (uint)polygonsPlotted, (uint)linesPlotted);
	}
	*/
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
QMenu *GeoFeatureLayer::menu(QMenu *parent) const {
	if ( _layerProperties.size() < 2 )
		return NULL;

	QMenu *menu = new QMenu(parent);

	size_t visibleCount = 0;
	vector<LayerProperties*>::const_iterator it = _layerProperties.begin();
	const LayerProperties *root = *it++;
	for ( ; it != _layerProperties.end(); ++it ) {
		if ( (*it)->parent != root ) continue;
		QAction *action = menu->addAction((*it)->name.c_str());
		action->setCheckable(true);
		action->setChecked((*it)->visible);
		if ( (*it)->visible ) ++visibleCount;
		action->setData(QVariant::fromValue<void*>(*it));
		connect(action, SIGNAL(toggled(bool)), this, SLOT(toggleFeatureVisibility(bool)));
	}

	// Add "Select all" and "Select none" options if more than 1 property
	// is available
	if ( _layerProperties.size() >= 2 ) {
		QAction *firstPropertyAction = menu->actions().first();

		// Select all
		QAction *allAction = new QAction(tr("Select all"), menu);
		allAction->setEnabled(visibleCount < _layerProperties.size());
		connect(allAction, SIGNAL(triggered()), this, SLOT(showFeatures()));
		menu->insertAction(firstPropertyAction, allAction);

		// Select none
		QAction *noneAction = new QAction(tr("Select none"), menu);
		noneAction->setEnabled(visibleCount > 0);
		connect(noneAction, SIGNAL(triggered()), this, SLOT(hideFeatures()));
		menu->insertAction(firstPropertyAction, noneAction);

		// Separator
		menu->insertSeparator(firstPropertyAction);
	}

	return menu;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GeoFeatureLayer::toggleFeatureVisibility(bool checked) {
	QAction *action = static_cast<QAction*>(sender());
	void *propertyPtr = action->data().value<void*>();
	LayerProperties *prop = reinterpret_cast<LayerProperties*>(propertyPtr);
	if ( prop == NULL )
		return;

	if ( prop->visible == checked ) return;

	prop->visible = checked;

	vector<Map::LayerProperties*>::const_iterator it;
	for ( it = _layerProperties.begin(); it != _layerProperties.end(); ++it ) {
		if ( !prop->isChild(*it) ) continue;
		(*it)->visible = checked;
	}

	emit updateRequested(RasterLayer);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GeoFeatureLayer::showFeatures() {
	setFeaturesVisibility(true);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GeoFeatureLayer::hideFeatures() {
	setFeaturesVisibility(false);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GeoFeatureLayer::initLayerProperites() {
	// Create a layer properties from BNA geo features
	const Geo::GeoFeatureSet &featureSet = Geo::GeoFeatureSetSingleton::getInstance();
	vector<Geo::Category*>::const_iterator itc = featureSet.categories().begin();
	for ( ; itc != featureSet.categories().end(); ++itc ) {
		// Initialize the base pen with the parent pen if available,
		// else use the default constructor
		Geo::Category *cat = *itc;
		LayerProperties *props = cat->parent == 0 ?
			new LayerProperties(cat->name.c_str()) :
			new LayerProperties(cat->name.c_str(), _layerProperties.at(cat->parent->id));
		_layerProperties.push_back(props);
		readLayerProperties(props, cat->dataDir);
	}

	const Geo::PolyRegions &fepRegions = Regions::polyRegions();
	if ( fepRegions.regionCount() > 0 ) {
		// Add empty root property if not exists yet
		if ( _layerProperties.empty() ) {
			_layerProperties.push_back(new LayerProperties(""));
			readLayerProperties(_layerProperties.front());
		}
		// Add fep properties
		_layerProperties.push_back(new LayerProperties("fep", _layerProperties.front()));
		readLayerProperties(_layerProperties.back(), fepRegions.dataDir());
	}

	// Build legends
	for ( size_t i = 0; i < _layerProperties.size(); ++i ) {
		LayerProperties *prop = _layerProperties[i];
		if ( !prop->title.empty() ) {
			StandardLegend *legend = new StandardLegend(this);
			legend->setTitle(prop->title.c_str());
			legend->setArea(prop->legendArea);
			legend->setOrientation(prop->orientation);

			QVector<LayerProperties*> items;

			// Find all child labels
			for ( size_t j = 0; j < _layerProperties.size(); ++j ) {
				LayerProperties *child = _layerProperties[j];
				if ( !child->label.empty() && prop->isChild(child) )
					items.append(child);
			}

			qSort(items.begin(), items.end(), compareByIndex);

			for ( int j = 0; j < items.count(); ++j ) {
				if ( items[j]->filled )
					legend->addItem(new StandardLegendItem(items[j]->pen,
					                                       items[j]->brush,
					                                       items[j]->label.c_str()));
				else
					legend->addItem(new StandardLegendItem(items[j]->pen,
					                                       items[j]->label.c_str()));
			}

			addLegend(legend);
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void GeoFeatureLayer::setFeaturesVisibility(bool visible) {
	bool updateRequired = false;
	vector<LayerProperties*>::const_iterator it = _layerProperties.begin();
	for ( ; it != _layerProperties.end(); ++it ) {
		if ( (*it)->visible != visible ) {
			(*it)->visible = visible;
			updateRequired = true;
		}
	}

	if ( updateRequired ) {
		emit updateRequested(RasterLayer);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
