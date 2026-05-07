#include "UI/Layout/DockTree.h"

namespace UI
{
    DockTree::DockTree()
        : m_root(makeNode())
    {
    }

    DockNode* DockTree::root()
    {
        return m_root.get();
    }

    const DockNode* DockTree::root() const
    {
        return m_root.get();
    }

    bool DockTree::empty() const
    {
        return !m_root || m_root->isEmpty();
    }

    void DockTree::clear()
    {
        m_root = makeNode();
        m_root->setBounds(m_bounds);
    }

    const Rect& DockTree::bounds() const
    {
        return m_bounds;
    }

    void DockTree::setBounds(const Rect& bounds)
    {
        m_bounds = bounds;

        if (m_root)
        {
            m_root->setBounds(bounds);
        }
    }

    DockNode* DockTree::findNode(int nodeId)
    {
        return m_root ? m_root->findNode(nodeId) : nullptr;
    }

    const DockNode* DockTree::findNode(int nodeId) const
    {
        return m_root ? m_root->findNode(nodeId) : nullptr;
    }

    DockNode* DockTree::findContent(const std::string& contentId)
    {
        return m_root ? m_root->findContent(contentId) : nullptr;
    }

    const DockNode* DockTree::findContent(const std::string& contentId) const
    {
        return m_root ? m_root->findContent(contentId) : nullptr;
    }

    int DockTree::attachRoot(DockContentDescriptor content)
    {
        if (!m_root)
        {
            m_root = makeNode();
            m_root->setBounds(m_bounds);
        }

        m_root->attachContent(std::move(content));
        return m_root->id();
    }

    bool DockTree::splitNode(
        int nodeId,
        DockSplitOrientation orientation,
        float ratio,
        DockContentDescriptor newContent,
        bool newContentInFirstChild)
    {
        DockNode* target = findNode(nodeId);
        if (!target || !target->isLeaf())
        {
            return false;
        }

        DockContentDescriptor existingContent = target->detachContent();

        std::unique_ptr<DockNode> first = makeNode();
        std::unique_ptr<DockNode> second = makeNode();

        if (newContentInFirstChild)
        {
            first->attachContent(std::move(newContent));
            second->attachContent(std::move(existingContent));
        }
        else
        {
            first->attachContent(std::move(existingContent));
            second->attachContent(std::move(newContent));
        }

        return target->split(
            orientation,
            ratio,
            std::move(first),
            std::move(second));
    }

    DockContentDescriptor DockTree::detachContent(const std::string& contentId)
    {
        DockNode* node = findContent(contentId);
        if (!node)
        {
            return DockContentDescriptor{};
        }

        return node->detachContent();
    }

    std::vector<DockLayoutRecord> DockTree::createLayoutRecords() const
    {
        std::vector<DockLayoutRecord> records;

        if (m_root)
        {
            m_root->collectLayoutRecords(records);
        }

        return records;
    }

    int DockTree::allocateNodeId()
    {
        return m_nextNodeId++;
    }

    std::unique_ptr<DockNode> DockTree::makeNode()
    {
        return std::make_unique<DockNode>(allocateNodeId());
    }
}