#include "FlowScene.hpp"

#include <stdexcept>
#include <utility>

#include <QtWidgets/QGraphicsSceneMoveEvent>
#include <QtWidgets/QFileDialog>
#include <QtCore/QByteArray>
#include <QtCore/QBuffer>
#include <QtCore/QDataStream>
#include <QtCore/QFile>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QtGlobal>
#include <QtCore/QDebug>

#include "../node/Node.hpp"
#include "../node/NodeGraphicsObject.hpp"

#include "../node/NodeGraphicsObject.hpp"
#include "../connection/ConnectionGraphicsObject.hpp"

#include "../connection/Connection.hpp"

#include "FlowView.hpp"
#include "../DataModelRegistry.hpp"

using QtNodes::FlowScene;
using QtNodes::Node;
using QtNodes::NodeGraphicsObject;
using QtNodes::Connection;
using QtNodes::DataModelRegistry;
using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::TypeConverter;

namespace QtNodes
{
	FlowScene::FlowScene(std::shared_ptr<DataModelRegistry> registry, QObject* parent)
		: QGraphicsScene(parent)
		, m_registry(std::move(registry))
	{
		setItemIndexMethod(QGraphicsScene::NoIndex);

		// This connection should come first
		connect(this, &FlowScene::connectionCreated, this, &FlowScene::setupConnectionSignals);
		connect(this, &FlowScene::connectionCreated, this, &FlowScene::sendConnectionCreatedToNodes);
		connect(this, &FlowScene::connectionDeleted, this, &FlowScene::sendConnectionDeletedToNodes);
	}

	FlowScene::
		FlowScene(QObject* parent)
		: FlowScene(std::make_shared<DataModelRegistry>(),
			parent)
	{}


	FlowScene::~FlowScene()
	{
		clearScene();
	}

	std::shared_ptr<Connection> FlowScene::createConnection(PortType connectedPort, Node& node, PortIndex portIndex)
	{
		auto connection = std::make_shared<Connection>(connectedPort, node, portIndex);

		auto cgo = detail::make_unique<ConnectionGraphicsObject>(*this, *connection);

		// after this function connection points are set to node port
		connection->setGraphicsObject(std::move(cgo));

		m_connections[connection->id()] = connection;

		// Note: this connection isn't truly created yet. It's only partially created.
		// Thus, don't send the connectionCreated(...) signal.

		connect(connection.get(), &Connection::connectionCompleted, this, [this](Connection const& c) { connectionCreated(c); });

		return connection;
	}

	std::shared_ptr<Connection> FlowScene::createConnection(Node& nodeIn, PortIndex portIndexIn, Node& nodeOut, PortIndex portIndexOut, TypeConverter const& converter)
	{
		auto connection = std::make_shared<Connection>(nodeIn, portIndexIn, nodeOut, portIndexOut, converter);

		auto cgo = detail::make_unique<ConnectionGraphicsObject>(*this, *connection);

		nodeIn.nodeState().setConnection(PortType::In, portIndexIn, *connection);
		nodeOut.nodeState().setConnection(PortType::Out, portIndexOut, *connection);

		// after this function connection points are set to node port
		connection->setGraphicsObject(std::move(cgo));

		// trigger data propagation
		nodeOut.onDataUpdated(portIndexOut);

		m_connections[connection->id()] = connection;

		connectionCreated(*connection);

		return connection;
	}

	std::shared_ptr<Connection> FlowScene::restoreConnection(QJsonObject const& connectionJson)
	{
		QUuid nodeInId = QUuid(connectionJson["in_id"].toString());
		QUuid nodeOutId = QUuid(connectionJson["out_id"].toString());

		PortIndex portIndexIn = connectionJson["in_index"].toInt();
		PortIndex portIndexOut = connectionJson["out_index"].toInt();

		if (m_nodes.find(nodeInId) == m_nodes.end() || m_nodes.find(nodeOutId) == m_nodes.end())
			return nullptr;

		auto nodeIn = m_nodes[nodeInId].get();
		auto nodeOut = m_nodes[nodeOutId].get();

		auto getConverter = [&]()
		{
			QJsonValue converterVal = connectionJson["converter"];

			if (!converterVal.isUndefined())
			{
				QJsonObject converterJson = converterVal.toObject();

				NodeDataType inType{ converterJson["in"].toObject()["id"].toString().toStdString().c_str(),
									converterJson["in"].toObject()["name"].toString().toStdString().c_str() };

				NodeDataType outType{ converterJson["out"].toObject()["id"].toString().toStdString().c_str(),
										converterJson["out"].toObject()["name"].toString().toStdString().c_str() };

				auto converter = registry().getTypeConverter(outType, inType);
				if (converter)
					return converter;
			}

			return TypeConverter{};
		};

		std::shared_ptr<Connection> connection = createConnection(*nodeIn, portIndexIn, *nodeOut, portIndexOut, getConverter());

		// Note: the connectionCreated(...) signal has already been sent
		// by createConnection(...)

		return connection;
	}


	void FlowScene::deleteConnection(Connection& connection)
	{
		auto it = m_connections.find(connection.id());
		if (it != m_connections.end())
		{
			connection.removeFromNodes();
			m_connections.erase(it);
		}
	}


	Node& FlowScene::createNode(std::unique_ptr<NodeDataModel>&& dataModel)
	{
		dataModel->setScene(this);

		auto node = detail::make_unique<Node>(std::move(dataModel));
		auto ngo = detail::make_unique<NodeGraphicsObject>(*this, *node);

		node->setGraphicsObject(std::move(ngo));

		auto nodePtr = node.get();
		m_nodes[node->id()] = std::move(node);

		nodeCreated(*nodePtr);
		return *nodePtr;
	}

	Node* FlowScene::restoreNode(QJsonObject const& nodeJson)
	{
		QString modelName = nodeJson["model"].toObject()["name"].toString();

		auto dataModel = registry().create(modelName);
		if (!dataModel)
			return nullptr;

		dataModel->setScene(this);

		auto node = detail::make_unique<Node>(std::move(dataModel));
		auto ngo = detail::make_unique<NodeGraphicsObject>(*this, *node);
		node->setGraphicsObject(std::move(ngo));

		node->restore(nodeJson);

		auto nodePtr = node.get();
		m_nodes[node->id()] = std::move(node);

		nodePlaced(*nodePtr);
		nodeCreated(*nodePtr);
		return nodePtr;
	}

	void FlowScene::removeNode(Node& node)
	{
		removeNodeForcely(node);
	}

	void FlowScene::removeNodeForcely(Node& node)
	{
		// call signal
		nodeDeleted(node);

		for (auto portType : { PortType::In,PortType::Out })
		{
			auto nodeState = node.nodeState();
			auto const& nodeEntries = nodeState.getEntries(portType);

			for (auto& connections : nodeEntries)
			{
				for (auto const& pair : connections)
					deleteConnection(*pair.second);
			}
		}

		m_nodes.erase(node.id());
	}

	DataModelRegistry& FlowScene::registry() const
	{
		return *m_registry;
	}

	void FlowScene::setRegistry(std::shared_ptr<DataModelRegistry> registry)
	{
		m_registry = std::move(registry);
	}

	void FlowScene::iterateOverNodes(std::function<void(Node*)> const& visitor)
	{
		for (const auto& _node : m_nodes)
		{
			visitor(_node.second.get());
		}
	}

	void FlowScene::iterateOverNodeData(std::function<void(NodeDataModel*)> const& visitor)
	{
		for (const auto& _node : m_nodes)
		{
			visitor(_node.second->nodeDataModel());
		}
	}

	void FlowScene::iterateOverNodeDataDependentOrder(std::function<void(NodeDataModel*)> const& visitor)
	{
		std::set<QUuid> visitedNodesSet;

		//A leaf node is a node with no input ports, or all possible input ports empty
		auto isNodeLeaf = [](Node const& node, NodeDataModel const& model)
		{
			for (unsigned int i = 0; i < model.nPorts(PortType::In); ++i)
			{
				auto connections = node.nodeState().connections(PortType::In, i);
				if (!connections.empty())
				{
					return false;
				}
			}

			return true;
		};

		//Iterate over "leaf" nodes
		for (auto const& _node : m_nodes)
		{
			auto const& node = _node.second;
			auto model = node->nodeDataModel();

			if (isNodeLeaf(*node, *model))
			{
				visitor(model);
				visitedNodesSet.insert(node->id());
			}
		}

		auto areNodeInputsVisitedBefore = [&](Node const& node, NodeDataModel const& model)
		{
			for (size_t i = 0; i < model.nPorts(PortType::In); ++i)
			{
				auto connections = node.nodeState().connections(PortType::In, i);

				for (auto& conn : connections)
				{
					if (visitedNodesSet.find(conn.second->getNode(PortType::Out)->id()) == visitedNodesSet.end())
					{
						return false;
					}
				}
			}

			return true;
		};

		//Iterate over dependent nodes
		while (m_nodes.size() != visitedNodesSet.size())
		{
			for (auto const& _node : m_nodes)
			{
				auto const& node = _node.second;
				if (visitedNodesSet.find(node->id()) != visitedNodesSet.end())
					continue;

				auto model = node->nodeDataModel();

				if (areNodeInputsVisitedBefore(*node, *model))
				{
					visitor(model);
					visitedNodesSet.insert(node->id());
				}
			}
		}
	}

	QPointF FlowScene::getNodePosition(const Node& node) const
	{
		return node.nodeGraphicsObject().pos();
	}

	void FlowScene::setNodePosition(Node& node, const QPointF& pos) const
	{
		node.nodeGraphicsObject().setPos(pos);
		node.nodeGraphicsObject().moveConnections();
	}

	QSizeF FlowScene::getNodeSize(const Node& node) const
	{
		return QSizeF(node.nodeGeometry().width(), node.nodeGeometry().height());
	}

	std::unordered_map<QUuid, std::unique_ptr<Node> > const& FlowScene::nodes() const
	{
		return m_nodes;
	}

	std::unordered_map<QUuid, std::shared_ptr<Connection> > const& FlowScene::connections() const
	{
		return m_connections;
	}

	std::vector<Node*>FlowScene::allNodes() const
	{
		std::vector<Node*> nodes;

		std::transform(m_nodes.begin(),
			m_nodes.end(),
			std::back_inserter(nodes),
			[](std::pair<QUuid const, std::unique_ptr<Node>> const& p) { return p.second.get(); });

		return nodes;
	}

	std::vector<Node*> FlowScene::selectedNodes() const
	{
		QList<QGraphicsItem*> graphicsItems = selectedItems();

		std::vector<Node*> ret;
		ret.reserve(graphicsItems.size());

		for (QGraphicsItem* item : graphicsItems)
		{
			auto ngo = qgraphicsitem_cast<NodeGraphicsObject*>(item);

			if (ngo != nullptr)
			{
				ret.push_back(&ngo->node());
			}
		}

		return ret;
	}

	void FlowScene::clearScene()
	{
		//Manual node cleanup. Simply clearing the holding datastructures doesn't work, the code crashes when
		// there are both nodes and connections in the scene. (The data propagation internal logic tries to propagate
		// data through already freed connections.)
		while (m_connections.size() > 0)
		{
			deleteConnection(*m_connections.begin()->second);
		}

		while (m_nodes.size() > 0)
		{
			removeNode(*m_nodes.begin()->second);
		}
	}

	void FlowScene::save() const
	{
		QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Open Flow Scene"), QDir::homePath(), tr("Flow Scene Files (*.flow)"));
		if (!fileName.isEmpty())
		{
			if (!fileName.endsWith("flow", Qt::CaseInsensitive))
				fileName += ".flow";

			QFile file(fileName);
			if (file.open(QIODevice::WriteOnly))
			{
				file.write(saveToMemory());
			}
		}
	}

	void FlowScene::load()
	{
		clearScene();

		QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Open Flow Scene"), QDir::homePath(), tr("Flow Scene Files (*.flow)"), nullptr, QFileDialog::DontUseNativeDialog);
		if (!QFileInfo::exists(fileName))
			return;

		QFile file(fileName);

		if (!file.open(QIODevice::ReadOnly))
			return;

		QByteArray wholeFile = file.readAll();

		loadFromMemory(wholeFile);
	}

	QByteArray FlowScene::saveToMemory() const
	{
		QJsonObject sceneJson;

		QJsonArray nodesJsonArray;

		for (auto const& pair : m_nodes)
		{
			auto const& node = pair.second;

			nodesJsonArray.append(node->save());
		}

		sceneJson["nodes"] = nodesJsonArray;

		QJsonArray connectionJsonArray;
		for (auto const& pair : m_connections)
		{
			auto const& connection = pair.second;

			QJsonObject connectionJson = connection->save();

			if (!connectionJson.isEmpty())
				connectionJsonArray.append(connectionJson);
		}

		sceneJson["connections"] = connectionJsonArray;

		QJsonDocument document(sceneJson);

		return document.toJson();
	}

	void FlowScene::loadFromMemory(const QByteArray& data)
	{
		QJsonObject const jsonDocument = QJsonDocument::fromJson(data).object();

		QJsonArray nodesJsonArray = jsonDocument["nodes"].toArray();

		for (QJsonValueRef node : nodesJsonArray)
		{
			restoreNode(node.toObject());
		}

		QJsonArray connectionJsonArray = jsonDocument["connections"].toArray();

		for (QJsonValueRef connection : connectionJsonArray)
		{
			restoreConnection(connection.toObject());
		}
	}

	void FlowScene::setupConnectionSignals(Connection const& c)
	{
		connect(&c, &Connection::connectionMadeIncomplete, this, &FlowScene::connectionDeleted, Qt::UniqueConnection);
	}

	void FlowScene::sendConnectionCreatedToNodes(Connection const& c)
	{
		Node* from = c.getNode(PortType::Out);
		Node* to = c.getNode(PortType::In);

		Q_ASSERT(from != nullptr);
		Q_ASSERT(to != nullptr);

		from->nodeDataModel()->outputConnectionCreated(c);
		to->nodeDataModel()->inputConnectionCreated(c);
	}

	void FlowScene::sendConnectionDeletedToNodes(Connection const& c)
	{
		Node* from = c.getNode(PortType::Out);
		Node* to = c.getNode(PortType::In);

		Q_ASSERT(from != nullptr);
		Q_ASSERT(to != nullptr);

		from->nodeDataModel()->outputConnectionDeleted(c);
		to->nodeDataModel()->inputConnectionDeleted(c);
	}

	Node* locateNodeAt(QPointF scenePoint, FlowScene& scene, QTransform const& viewTransform)
	{
		// items under cursor
		QList<QGraphicsItem*> items = scene.items(scenePoint, Qt::IntersectsItemShape, Qt::DescendingOrder, viewTransform);

		//// items convertable to NodeGraphicsObject
		std::vector<QGraphicsItem*> filteredItems;

		std::copy_if(items.begin(), items.end(), std::back_inserter(filteredItems), [](QGraphicsItem* item)
		{
			return (dynamic_cast<NodeGraphicsObject*>(item) != nullptr);
		});

		Node* resultNode = nullptr;

		if (!filteredItems.empty())
		{
			QGraphicsItem* graphicsItem = filteredItems.front();
			auto ngo = dynamic_cast<NodeGraphicsObject*>(graphicsItem);

			resultNode = &ngo->node();
		}

		return resultNode;
	}
}

