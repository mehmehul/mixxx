#include <QtAlgorithms>

#include "controllers/controlleroutputmappingtablemodel.h"
#include "controllers/midi/midimessage.h"
#include "controllers/delegates/controldelegate.h"
#include "controllers/delegates/midichanneldelegate.h"
#include "controllers/delegates/midiopcodedelegate.h"
#include "controllers/delegates/midibytedelegate.h"

ControllerOutputMappingTableModel::ControllerOutputMappingTableModel(QObject* pParent)
        : ControllerMappingTableModel(pParent) {
}

ControllerOutputMappingTableModel::~ControllerOutputMappingTableModel() {
}

void ControllerOutputMappingTableModel::apply() {
    if (m_pMidiPreset != NULL) {
        // Clear existing output mappings and insert all the output mappings in
        // the table into the preset.
        m_pMidiPreset->outputMappings.clear();
        foreach (const MidiOutputMapping& mapping, m_midiOutputMappings) {
            m_pMidiPreset->outputMappings.insertMulti(mapping.control, mapping.output);
        }
    }
}

void ControllerOutputMappingTableModel::onPresetLoaded() {
    clear();

    if (m_pMidiPreset != NULL) {
        // TODO(rryan): Tooltips
        setHeaderData(MIDI_COLUMN_CHANNEL, Qt::Horizontal, tr("Channel"));
        setHeaderData(MIDI_COLUMN_OPCODE, Qt::Horizontal, tr("Opcode"));
        setHeaderData(MIDI_COLUMN_CONTROL, Qt::Horizontal, tr("Control"));
        setHeaderData(MIDI_COLUMN_ON, Qt::Horizontal, tr("On Value"));
        setHeaderData(MIDI_COLUMN_OFF, Qt::Horizontal, tr("Off Value"));
        setHeaderData(MIDI_COLUMN_ACTION, Qt::Horizontal, tr("Action"));
        setHeaderData(MIDI_COLUMN_MIN, Qt::Horizontal, tr("On Range Min"));
        setHeaderData(MIDI_COLUMN_MAX, Qt::Horizontal, tr("On Range Max"));
        setHeaderData(MIDI_COLUMN_COMMENT, Qt::Horizontal, tr("Comment"));

        beginInsertRows(QModelIndex(), 0, m_pMidiPreset->outputMappings.size() - 1);
        for (QHash<MixxxControl, MidiOutput>::const_iterator it =
                     m_pMidiPreset->outputMappings.begin();
             it != m_pMidiPreset->outputMappings.end(); ++it) {
            MidiOutputMapping mapping;
            mapping.control = it.key();
            mapping.output = it.value();
            m_midiOutputMappings.append(mapping);
        }
        endInsertRows();
    }
}

void ControllerOutputMappingTableModel::clear() {
    if (m_pMidiPreset != NULL) {
        beginRemoveRows(QModelIndex(), 0, m_midiOutputMappings.size() - 1);
        m_midiOutputMappings.clear();
        endRemoveRows();
    }
}

void ControllerOutputMappingTableModel::addEmptyMapping() {
    if (m_pMidiPreset != NULL) {
        beginInsertRows(QModelIndex(), m_midiOutputMappings.size(),
                        m_midiOutputMappings.size());
        m_midiOutputMappings.append(MidiOutputMapping());
        endInsertRows();
    }
}

void ControllerOutputMappingTableModel::removeMappings(QModelIndexList indices) {
    // Values don't matter, it's just to get a consistent ordering.
    QList<int> rows;
    foreach (const QModelIndex& index, indices) {
        rows.append(index.row());
    }
    qSort(rows);

    int lastRow = -1;
    while (!rows.empty()) {
        int row = rows.takeLast();
        if (row == lastRow) {
            continue;
        }

        beginRemoveRows(QModelIndex(), row, row);
        m_midiOutputMappings.removeAt(row);
        endRemoveRows();
    }
}

QAbstractItemDelegate* ControllerOutputMappingTableModel::delegateForColumn(
        int column, QWidget* pParent) {
    if (m_pMidiPreset != NULL) {
        switch (column) {
            case MIDI_COLUMN_CHANNEL:
                return new MidiChannelDelegate(pParent);
            case MIDI_COLUMN_OPCODE:
                return new MidiOpCodeDelegate(pParent);
            case MIDI_COLUMN_CONTROL:
            case MIDI_COLUMN_ON:
            case MIDI_COLUMN_OFF:
                return new MidiByteDelegate(pParent);
            case MIDI_COLUMN_ACTION:
                return new ControlDelegate(this);
        }
    }
    return NULL;
}

int ControllerOutputMappingTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    if (m_pMidiPreset != NULL) {
        return m_midiOutputMappings.size();
    }
    return 0;
}

int ControllerOutputMappingTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    // Control and description
    const int kBaseColumns = 2;
    if (m_pMidiPreset != NULL) {
        // Channel, Opcode, Control, On, Off, Min, Max
        return kBaseColumns + 7;
    }
    return 0;
}

QVariant ControllerOutputMappingTableModel::data(const QModelIndex& index,
                                                 int role) const {
    // We use UserRole as the "sort" role with QSortFilterProxyModel.
    if (!index.isValid() || (role != Qt::DisplayRole &&
                             role != Qt::EditRole &&
                             role != Qt::UserRole)) {
        return QVariant();
    }

    int row = index.row();
    int column = index.column();

    if (m_pMidiPreset != NULL) {
        if (row < 0 || row >= m_midiOutputMappings.size()) {
            return QVariant();
        }

        const MidiOutputMapping& mapping = m_midiOutputMappings.at(row);
        QString value;
        switch (column) {
            case MIDI_COLUMN_CHANNEL:
                return channelFromStatus(mapping.output.status);
            case MIDI_COLUMN_OPCODE:
                return opCodeFromStatus(mapping.output.status);
            case MIDI_COLUMN_CONTROL:
                return mapping.output.control;
            case MIDI_COLUMN_ON:
                return mapping.output.on;
            case MIDI_COLUMN_OFF:
                return mapping.output.off;
            case MIDI_COLUMN_MIN:
                return mapping.output.min;
            case MIDI_COLUMN_MAX:
                return mapping.output.max;
            case MIDI_COLUMN_ACTION:
                if (role == Qt::UserRole) {
                    // TODO(rryan): somehow get the delegate display text?
                    return mapping.control.group() + "," + mapping.control.item();
                }
                return qVariantFromValue(ConfigKey(mapping.control.group(),
                                                   mapping.control.item()));
            case MIDI_COLUMN_COMMENT:
                return mapping.control.description();
            default:
                return QVariant();
        }
    }
    return QVariant();
}

bool ControllerOutputMappingTableModel::setData(const QModelIndex& index,
                                                const QVariant& value,
                                                int role) {
    if (!index.isValid() || role != Qt::EditRole) {
        return false;
    }

    int row = index.row();
    int column = index.column();

    if (m_pMidiPreset != NULL) {
        if (row < 0 || row >= m_midiOutputMappings.size()) {
            return false;
        }

        MidiOutputMapping& mapping = m_midiOutputMappings[row];
        ConfigKey key;
        switch (column) {
            case MIDI_COLUMN_CHANNEL:
                mapping.output.status = static_cast<unsigned char>(
                    opCodeFromStatus(mapping.output.status)) |
                        static_cast<unsigned char>(value.toInt());
                emit(dataChanged(index, index));
                return true;
            case MIDI_COLUMN_OPCODE:
                mapping.output.status = static_cast<unsigned char>(
                    channelFromStatus(mapping.output.status)) |
                        static_cast<unsigned char>(value.toInt());
                emit(dataChanged(index, index));
                return true;
            case MIDI_COLUMN_CONTROL:
                mapping.output.control = static_cast<unsigned char>(value.toInt());
                emit(dataChanged(index, index));
                return true;
            case MIDI_COLUMN_ON:
                mapping.output.on = static_cast<unsigned char>(value.toInt());
                emit(dataChanged(index, index));
                return true;
            case MIDI_COLUMN_OFF:
                mapping.output.off = static_cast<unsigned char>(value.toInt());
                emit(dataChanged(index, index));
                return true;
            case MIDI_COLUMN_MIN:
                mapping.output.min = value.toDouble();
                emit(dataChanged(index, index));
                return true;
            case MIDI_COLUMN_MAX:
                mapping.output.max = value.toDouble();
                emit(dataChanged(index, index));
                return true;
            case MIDI_COLUMN_ACTION:
                key = qVariantValue<ConfigKey>(value);
                // TODO(rryan): nuke MixxxControl from orbit.
                mapping.control.setGroup(key.group);
                mapping.control.setItem(key.item);
                emit(dataChanged(index, index));
                return true;
            case MIDI_COLUMN_COMMENT:
                mapping.control.setDescription(value.toString());
                emit(dataChanged(index, index));
                return true;
            default:
                return false;
        }
    }

    return false;
}
